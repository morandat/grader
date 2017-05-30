#ifndef _XOPEN_SOURCE
# define _XOPEN_SOURCE 700
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <printf.h>
#include <string.h>

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <execinfo.h>

#ifdef __APPLE__
  printf_domain_t domain;
#endif
#include "grader.h"


#ifndef TIMEOUT
# define TIMEOUT 3
#endif

#define EXIT(_passed, _failed) exit((_passed) << 1 | (_failed))

#define HAS_FAILED(_status) (_status & 1)

extern const struct test exercice[];
char last_call_buffer[PRINTF_BUFFER_SIZE];

static const struct test *_current_test = NULL;
static int _assert_passed = 0;
static int _assert_failed = 0;
static int _user_assert = 0;
static int _barrier_enabled = 0;
static pid_t _current_child;
static enum grader_mode_t _mode = TEST;

static jmp_buf _global_test_trap, _local_test_trap;

static struct {
  void *sym;
  unsigned current_stack;
  struct call_info info;
} current_function;

void print_test_result(const struct test *test, int status) {
  int assert_passed = status >> 1;
  int failed = HAS_FAILED(status);
  fprintf(stderr, "** "COLOR(TEST, "Test %s")" **\t%s (%d)\n",
      test->name,
      failed ? COLOR(FAILED, "FAILED") : COLOR(PASSED, "PASSED"), assert_passed);
  fflush(stderr);
}

void print_raw_test_result(const struct test *test, int status) {
  (void) test;
  fprintf(stdout, "; %d ; %d", !(status & 1), status >> 1);
  fflush(stdout);
}

static int _failed_count;
static int _runned_count;
static struct {const struct test *test; int status;} _runned_tests[100]; // FIXME

void print_compact_test_result(const struct test *test, int status) {
  int assert_passed = status >> 1;
  int failed = HAS_FAILED(status);
  _runned_tests[_runned_count].test = test;
  _runned_tests[_runned_count++].status = status;
  if (failed) {
    _failed_count ++;
    if (assert_passed > 0)
        fprintf(stderr, "** "COLOR(TEST, "Test %s")" **\t"COLOR(FAILED, "FAILED")" (%d)\n",
                test->name, assert_passed);
    fflush(stderr);
  }
}

void print_compact_summary() {
  fprintf(stderr, "\n== "COLOR(TEST, "Summary")" ==\n");
  fprintf(stderr, COLOR(PASSED, "PASSED")" %d/%d\t", _runned_count - _failed_count, _runned_count);
  fprintf(stderr, ""COLOR(FAILED, "FAILED")" %d/%d\n", _failed_count, _runned_count);
  if (_failed_count != _runned_count) {
    fprintf(stderr, "Tests passed:");
    for (int i = 0; i < _runned_count; i ++)
      if (!HAS_FAILED(_runned_tests[i].status))
        fprintf(stderr, " %s(%d)" , _runned_tests[i].test->name, _runned_tests[i].status >> 1);
    fprintf(stderr, "\n");
  }
  if (_failed_count != 0) {
    fprintf(stderr, "Tests failed:");
    for (int i = 0; i < _runned_count; i ++)
      if (HAS_FAILED(_runned_tests[i].status))
        fprintf(stderr, " %s(%d)" , _runned_tests[i].test->name, _runned_tests[i].status >> 1);
    fprintf(stderr, "\n");
  }
}

struct printer print_raw = { .result = print_raw_test_result };
struct printer print_legacy = { .result = print_test_result };
struct printer print_compact = { .result = print_compact_test_result, .summary = print_compact_summary };

void print_trace() {
  size_t size;
  void *buffer[20];
  size = backtrace(buffer, 20);
  backtrace_symbols_fd(buffer, size, STDERR_FILENO);
}

static void segfault_msg(int signal) {
  fprintf(stderr,
      "[%s]\t"COLOR(FAILED, "Houston, we've got a problem")". Hit by signal '%s'.\n", _current_test->name, strsignal(signal));
  if (last_call_buffer[0])
    fprintf(stderr, "Last call: %s\n", last_call_buffer);
  fflush(stderr);
}

static void segfault(int signal, siginfo_t *siginfo, void *context) {
  (void) siginfo;
  (void) context;
  segfault_msg(signal);
  // print_trace();
  EXIT(_assert_passed, EXIT_FAILURE);
}

static void timeout(int signal) {
  (void) signal;
  fprintf(stderr,
      "[%s]\tTime's up ! There is probably an infinite loop somewhere.\n", _current_test->name);
    fflush(stderr);
  kill(_current_child, SIGTERM);
}

static void prepare_test(const struct test *test) {
  _current_test = test;
  _assert_passed = 0;
  signal(SIGALRM, &timeout);
}

static void set_timeout(pid_t pid) {
  _current_child = pid;
  alarm(TIMEOUT);
}


enum grader_mode_t get_grader_mode() {
    return _mode;
}

#define SEGV_STACK_SIZE 4096
static void install_sighandlers() {
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_flags = SA_SIGINFO|SA_ONSTACK;
    action.sa_sigaction = &segfault;
    sigaction(SIGSEGV, &action, NULL);
    sigaction(SIGBUS, &action, NULL);
    sigaction(SIGABRT, &action, NULL);

    stack_t segv_stack;
    if (posix_memalign(&segv_stack.ss_sp, sysconf(_SC_PAGESIZE), SEGV_STACK_SIZE)) {
        MSG(COLOR(TEST, "Not enough memory to allocate alternate stack"));
        segv_stack.ss_flags = 0;
        segv_stack.ss_size = SEGV_STACK_SIZE;
        sigaltstack(&segv_stack, NULL);
    }
}

static int wait_for_test_completion(pid_t pid)
{
    int status;
    set_timeout(pid);
    if (waitpid(pid, &status, 0) == -1)
        return(EXIT_FAILURE); // TODO
    alarm(0);
    if (WIFEXITED(status))
        return WEXITSTATUS(status);
    fprintf(stderr,
            "[%s]\t " COLOR(FAILED, "Untrapped signal on child, the stack was probably smashed")".\n", _current_test->name);
    segfault_msg(WTERMSIG(status));
    return 1;
}

static int execute_test(const struct test *test)
{
    pid_t pid;
    int status;
    switch(pid = fork()) {
        case -1:
            perror("Can not fork test");
            exit(EXIT_FAILURE);
        case 0: // Child
            dup2(STDERR_FILENO, STDOUT_FILENO);
            install_sighandlers();
            last_call_buffer[0] = 0;
            if (!setjmp(_global_test_trap)) {
                memcpy(_local_test_trap, _global_test_trap, sizeof(_local_test_trap));
                test->fun(test->len, test->param);
                status = _assert_failed;
            } else {
                fprintf(stderr,
                        "[%s]\t" COLOR(FAILED, "%s was not expected.\n"),
                        _current_test->name, _user_assert ? "Assertion" : "Early exit()");
                fflush(stderr);
                status = EXIT_FAILURE; // We could use the error code of set jmp ... but who cares
            }
            EXIT(_assert_passed, status != 0);
        default: // Parent
            return wait_for_test_completion(pid);
    }
}

const struct test *find_test(const struct test *tests, const char *name) {
  int i = -1;
  while(tests[++i].name)
    if (!strcmp(tests[i].name, name))
        return &tests[i];
  return NULL;
}

void assert_passed() {
  _assert_passed ++;
}
void assert_failed() {
  _assert_failed ++;
}

void reset_user_assert() {
    _user_assert = 0;
}

int user_has_asserted() {
    return _user_assert != 0;
}

void register_barrier(jmp_buf checkpoint) {
    // TODO check that _local_test_trap == _global_test_trap
    memcpy(_local_test_trap, checkpoint, sizeof(_local_test_trap));
    _barrier_enabled = 1;
}

void release_barrier() {
    memcpy(_local_test_trap, _global_test_trap, sizeof(_local_test_trap));
    _barrier_enabled = 0;
}

void grader_exit(int _error_code) __attribute__ ((noreturn));

void grader_exit(int _error_code) {
    longjmp(_local_test_trap,
            (_error_code >= 0) ? _error_code + 1 : _error_code - 1);
}

void grader_assert_fail(const char *__assertion, const char *__file, unsigned int __line, const char *__function) {
    if (!_barrier_enabled) {
        fprintf(stderr, "assert: %s:%u %s:  Assertion `%s` failed.\n",
                __file, __line, __function, __assertion);
        fflush(stderr);
    }
    _user_assert = 1;
    grader_exit(1);
}

// Fucking GNU_EXTENSION !
void grader_assert_perror_fail(int __errnum, const char *__file, unsigned int __line, const char *__function) {
    if (!_barrier_enabled) {
        fprintf(stderr, "assert: %s:%u %s:  Unexpected error: %s.\n",
                __file, __line, __function,  strerror(__errnum));
        fflush(stderr);
    }
    _user_assert = 1;
    grader_exit(1);
}

int run_test(const struct test *test, const struct printer *print) {
  prepare_test(test);
  int status = execute_test(test);
  print->result(test, status);
  return status & 1;
}

void list_tests(const struct test *tests) {
  for (int i = 0; tests[i].name ; i ++)
    printf ("%s\n", tests[i].name);
}

int run_test_group(const struct test tests[], const struct printer *print) {
  int ret = EXIT_SUCCESS;
  if (print->header)
    print->header();
  for (int i = 0; tests[i].name ; i ++)
    ret |= run_test(&tests[i], print);
  if (print->summary)
    print->summary();
  return ret;
}

// Buffer for printf grabbing
static int printf_buffer_pos = -1;
static char printf_buffer[PRINTF_BUFFER_SIZE + 1];

void start_grab_printf() {
  printf_buffer_pos = 0;
}

char* end_grab_printf() {
  if (printf_buffer_pos < 0) {
    printf_buffer_pos = 0;
    fprintf(stderr, "end_grab_printf(), can't be called before start_grab_printf()\n");
    fflush(stderr);
  }

  printf_buffer[printf_buffer_pos] = 0;
  printf_buffer_pos = -1;
  return printf_buffer;
}

int printf(const char * restrict format, ...) {
  int res;
  va_list ap;
  va_start(ap, format);
  if (printf_buffer_pos >= 0) {
    // Grab printf => call snprintf
    res = vsnprintf(printf_buffer + printf_buffer_pos, PRINTF_BUFFER_SIZE - printf_buffer_pos, format, ap);
    printf_buffer_pos += res;
  } else {
    // fallback to classical printf
    res = vprintf(format, ap);
  }
  va_end(ap);
  return res;
}

int puts(const char *s) {
  int res = 1;
  if (printf_buffer_pos >= 0) {
    char *bstart = printf_buffer + printf_buffer_pos;
    char *bend = stpcpy(bstart, s);
    printf_buffer_pos += bend - bstart;
  } else {
    res = fputs(s, stdout);
    res = fputs("\n", stdout);
  }
  return res;
}

struct call_info trace_function(void *sym) {
  struct call_info new_info = {0, 0};
  struct call_info last = current_function.info;
  current_function.sym = sym;
  current_function.current_stack = 0;
  current_function.info = new_info;
  return last;
}

void __cyg_profile_func_enter (void *this_fn, void *call_site) {
  (void) call_site;
  if (this_fn == current_function.sym) {
    current_function.info.count ++;
    if (++current_function.current_stack > current_function.info.stack_depth)
      current_function.info.stack_depth = current_function.current_stack;
  }
}

void __cyg_profile_func_exit  (void *this_fn, void *call_site) {
  (void) call_site;
  if (this_fn == current_function.sym)
    current_function.current_stack --;
}

// This is a debug function, it can be used but the prtotype is not in grader.h
void dump_a_stat(const struct a_stats s) {
    printf("a:%lu f:%lu r:%lu e:%lu\n",
            s.allocated, s.freed, s.reallocated, s.fault);
}

WEAK_DEFINE(alt_main, int, int argc, char** argv) {
  (void) argc;
  (void) argv;
  printf("'main' is not defined\n");
  return EXIT_SUCCESS;
}

extern int register_printf_handlers();

void test_local_setup_default() { }
void test_local_setup() __attribute__ ((weak, alias("test_local_setup_default")));

void test_each_setup_default() { }
void test_each_setup() __attribute__ ((weak, alias("test_each_setup_default")));

int grader(const struct test *exam, int argc, char **argv) {
  int ch;
  int status = EXIT_SUCCESS;
  const struct printer *printer = &print_compact;

  if (register_printf_handlers()) {
    perror("Unable to init grader lib, OS or compiler not supported");
    return EXIT_FAILURE;
  }

  while ((ch = getopt(argc, argv, "mcrh?lL")) != -1) {
    switch (ch) {
      case 'c':
      case 'r':
        _mode = GRADE;
        break;
      case 'L':
        printer = &print_legacy;
        break;
      case 'm':
        _mode = MAIN;
        break;
      default:
        list_tests(exam);
        return EXIT_FAILURE;
    }
  }
  argc -= optind;
  argv += optind;

  switch (_mode) {
    case MAIN:
      *(argv - 1) = "exercice"; // FIXME
      status = alt_main(argc + 1, argv - 1);
      break;
    case GRADE:
      printer = &print_raw;
    case TEST:
      if (!*argv) {
        status = run_test_group(exam, printer);
      } else while (*argv++)  {
        const struct test *test;
        if ((test = find_test(exam, argv[-1])))
          status |= run_test(test, printer);
        else
          fprintf(stderr, "Test '%s' does not exists ", argv[-1]);
      }
      break;
    default:
      MSG("Grader internal error: unknown mode %d", _mode);
  }
  return status;
}

int normalize(int v) {
  if (v > 0) return 1;
  if (v < 0) return -1;
  return 0;
}

int compare_memory(const void* v1, const void* v2, size_t size)
{
    return !memcmp(v1, v2, size);
}

int array_compare(int(*comparator)(const void*, const void*, size_t),
        const void *a1, const void *a2, size_t n, size_t item_size)
{
  const char *c1 = a1;
  const char *c2 = a2;
  for (size_t i = 0; i < n; i ++) // FIXME comparator should now be compatible with memcmp
    if (comparator(c1 + i*item_size, c2 + i*item_size, item_size) == 0)
      return i;
  return -1;
}

int array_compare_unordered(int (*comparator)(const void*, const void*),
    const void *a1, const void *a2, size_t n, size_t item_size)
{
  size_t n2 = n;
  const void *v2[n];
  const char *c1 = a1;
  for (size_t i = 0; i < n; i ++)
    v2[i] = a2 + i*item_size;

  for (size_t i = 0; i < n; i ++) {
    for (size_t j = 0; j < n2; j ++) {
      if (comparator(c1 + i*item_size, v2[j])) {
        n2--;
        if (n2) v2[n2] = v2[j];
        goto next;
      }
    }
    return i;
next:
    continue;
  }
  return -1;
}

int as_bool(int v) {
    return v != 0;
}

size_t strnlen(const char *s, size_t maxlen)
{
    size_t size = 0;
    while (maxlen-- && *s++)
        size ++;
    return size;
}

int main(int argc, char **argv) {
    return grader(exercice, argc, argv);
}
