#ifndef __GRADER_H
#define __GRADER_H

#include <stdio.h>
#include <string.h>
#include <printf.h>
#include <stdlib.h>
#include <setjmp.h>

////////////////////////////////////////////////////////////////
// Color macros
#define TEST_COLOR "35;1"
#define RESET_COLOR "0"
#define PASSED_COLOR "32"
#define FAILED_COLOR "31;1"
#define WARN_COLOR "34;1"

#ifndef NO_COLOR
# define COLOR(__c, __s) ESCAPE(__c) __s ESCAPE(RESET)
# define ESCAPE(__s) "\x1B[" __s##_COLOR "m"
#else
# define COLOR(__c, __s) __s
# define ESCAPE(__s)
#endif

#ifndef PRINTF_BUFFER_SIZE
# define PRINTF_BUFFER_SIZE 4096
#endif

////////////////////////////////////////////////////////////////
// Describe the call that will be checked
// DESCRIBE(__call, __printf_fmt_string, [__printf_arguments]+)
//
// typical usage:
// int actual = DESCRIBE(foo(_.x, _.y),
//                      "foo(%d, %S)", _.x, _.x, _.y);
// See XPRINTF
// (FIXME this uses gcc extension)
#define DESCRIBE(__call, ...)\
  ({ LAST_CALL(__VA_ARGS__); __call ;})


////////////////////////////////////////////////////////////////
// Assertions
////////////////////////////////////////////////////////////////
// Asserting x == y
//
// ASSERT_EXACT     (format,  expected_val, actual_val, tag)
// ASSERT_EXACT_MSG (expected_val, actual_val, msg, [msg_args]+)
//
// format: is the formatting string used to print the values
// tag: will be inserted in the message. Se AS_XXXX macro for predefined msgs
// msg: is a format string that will be printed if assertion fails
// msg_args: are arguments for format string
//
// You can use the special variable last_call_buffer ("%s") to display the
// last described call.

#define ASSERT_EXACT(__fmt, __expected, __actual, __tag)              \
    ASSERT_EXACT_MSG(__expected, __actual,                            \
        "Expecting %s'"__fmt"' but found '"__fmt"'. Called by:\n\t%s",\
        __tag, __expected, __actual, last_call_buffer)

#define ASSERT_EXACT_MSG(__expected, __actual, ...)     \
    do {                                                \
        if ((__expected) != (__actual))                 \
            FAILED(__VA_ARGS__);                        \
        else                                            \
            PASSED();                                   \
    } while (0)

////////////////////////////////////////////////////////////////
// Asserting x~= y
//
// ASSERT_FP        (format,  expected_val, actual_val, precision, tag)
// ASSERT_FP_MSG    (expected_val, actual_val, precision, msg, [msg_args]+)
//
// assertion for floating point values, asserting equality up to a
// precision in absolute value. See ASSERT_EXACT for arguments description

#define ASSERT_FP(__fmt, __expected, __actual, __prec, __tag)           \
    ASSERT_FP_MSG(__expected, __actual, __prec,                         \
        "Expecting %s'"__fmt"', but found '"__fmt"'. Called by:\n\t%s", \
        __tag,  __expected, __actual, last_call_buffer)

#define ASSERT_FP_MSG(__expected, __actual, __prec, ...)    \
    do {                                                    \
        if (fabs((__expected) - (__actual)) > (__prec))     \
            FAILED(__VA_ARGS__);                            \
        else                                                \
            PASSED();                                       \
    } while (0)

// Tags
#define AS_RETURN "as return value "
#define AS_RESULT "as return value "
#define AS_PARAM(__x) "in parameter `"#__x"` "
#define AS_CONTAINED_BY(__x) __x" to contain "
#define AS_PRINTED "to be printed "

// Primitives types
#define ASSERT_INT(__expected, __actual, __tag) \
    ASSERT_EXACT("%d", __expected, __actual, __tag)
#define ASSERT_UINT(__expected, __actual, __tag) \
    ASSERT_EXACT("%u", __expected, __actual, __tag)
#define ASSERT_LONG(__expected, __actual, __tag) \
    ASSERT_EXACT("%ld", __expected, __actual, __tag)
#define ASSERT_ULONG(__expected, __actual, __tag) \
    ASSERT_EXACT("%lu", __expected, __actual, __tag)
#define ASSERT_LONGLONG(__expected, __actual, __tag) \
    ASSERT_EXACT("%lld", __expected, __actual, __tag)
#define ASSERT_ULONGLONG(__expected, __actual, __tag) \
    ASSERT_EXACT("%llx", __expected, __actual, __tag)
#define ASSERT_FLOAT(__expected, __actual, __prec, __tag) \
    ASSERT_FP("%f", __expected, __actual, __prec, __tag)
#define ASSERT_DOUBLE(__expected, __actual, __prec, __tag) \
    ASSERT_FP("%lf", __expected, __actual, __prec, __tag)
#define ASSERT_CHAR(__expected, __actual, __tag)                      \
    ASSERT_EXACT_MSG(__expected, __actual,                        \
        "Expecting %s'%c' (code %hhd), but found '%c' (code %hhd). Called by:\n\t%s", \
        __tag, __expected, __expected, __actual, __actual, last_call_buffer)

///// Pointers
#define ASSERT_PTR(__expected, __actual, __tag) \
    ASSERT_EXACT("%p", __expected, __actual, __tag)
#define ASSERT_NULL(__actual, __tag) \
    ASSERT_EXACT("%p", NULL, __actual, __tag)
#define ASSERT_NOT_NULL(__actual, __tag)                                    \
    ASSERT_NOT_NULL_MSG(__actual, "Expected not null %s, but found %p. Called by:\n\t%s",\
          __tag, __actual, last_call_buffer)
#   define ASSERT_NOT_NULL_MSG(__actual, ...) \
    do {                                      \
        if (NULL == (__actual))               \
            FAILED(__VA_ARGS__);              \
        else                                  \
            PASSED();                         \
    } while (0)

///// Strings
#   define ASSERT_STRING(__expected, __actual, __tag)                         \
    ASSERT_STRING_MSG(__expected, __actual,                                   \
        "Expecting %s:\n\t\"%s\" (%zu)\nbut found\n\t\"%s\" (%zu)\nCalled by:\n\t%s",\
        __tag, __expected, strlen(__expected),                                \
        __actual, strnlen(__actual, strlen(__expected) + 10), last_call_buffer)
#   define ASSERT_STRING_MSG(__expected, __actual, ...)           \
    do {                                                          \
        if (memcmp(__expected, __actual, strlen(__expected) + 1)) \
            FAILED(__VA_ARGS__);                                  \
        else                                                      \
            PASSED();                                             \
    } while (0)

// Booleans
#   define ASSERT_FALSE(__actual, __tag)                               \
    ASSERT_EXACT_MSG(0, __actual, "Expecting %sfalse (zero), but found %d. Called by:\n\t%s",\
            __tag, __actual, last_call_buffer)
#   define ASSERT_TRUE(__actual, __tag) \
    ASSERT_TRUE_MSG(__actual, "Expecting %strue (not zero), but found %d. Called by:\n\t%s",\
            __tag, __actual, last_call_buffer)
#   define ASSERT_TRUE_MSG(__actual, ...) \
    do {                                  \
        if (!(__actual))                  \
            FAILED(__VA_ARGS__);          \
        else                              \
            PASSED();                     \
    } while (0)

// Structures
#   define ASSERT_STRUCT(__struct_comparator, __expected, __actual, __tag)  \
    ASSERT_STRUCT_MSG(__struct_comparator, __expected, __actual,            \
            "Expecting %s:\n\t%-@\nbut found:\n\t%-@\nCalled by:\n\t%s",      \
            __tag, 1, (__expected), 1, (__actual), last_call_buffer)
#   define ASSERT_STRUCT_MSG(__struct_comparator, __expected, __actual, ...)\
    do {                                                                    \
        if (!(((__expected) == (__actual)) ||                               \
            ((__actual) && (__expected) && __struct_comparator((__expected), (__actual))))) \
            FAILED(__VA_ARGS__);                                            \
        else                                                                \
            PASSED();                                                       \
    } while (0)

// Arrays
#define ASSERT_ARRAY(__size, __expected, __actual, __fmt, __tag)                          \
  do {                                                                                    \
    int __idx = array_compare((int (*) (const void*, const void*, size_t))compare_memory, \
            __expected, __actual, __size, sizeof(__expected[0]));                         \
    if (__idx != -1)                                                                      \
      FAILED("Expecting %s"__fmt" at index %d, but found "__fmt". Called by:\n\t%s",      \
          __tag, __expected[__idx], __idx, __actual[__idx], last_call_buffer);            \
    else                                                                                  \
      PASSED();                                                                           \
  } while (0)
#define ASSERT_STRUCT_ARRAY(__comparator, __size, __expected, __actual, __tag)          \
  do {                                                                                  \
    int __idx = array_compare((int (*) (const void*, const void*, size_t))__comparator, \
            __expected, __actual, __size, sizeof(__expected[0]));                       \
    if (__idx != -1)                                                                    \
      FAILED("Expecting %s:\n\t%-@\nat index %d, but found\n\t%-@\nCalled by:\n\t%s",   \
          __tag, 1, &__expected[__idx], __idx, 1, &__actual[__idx], last_call_buffer);  \
    else                                                                                \
      PASSED();                                                                         \
  } while (0)

#define ASSERT_ARRAY_UNORDERED(__size, __expected, __actual, __fmt, __tag)              \
  do {                                                                                  \
    int __idx = array_compare_unordered((int (*) (const void*, const void*, size_t))compare_memory, \
            __expected, __actual, __size, sizeof(__expected[0]));                       \
    if (__idx != -1)                                                                    \
      FAILED("Expecting %s"__fmt" to be found in array. Called by:\n\t%s",              \
          __tag, __expected[__idx], last_call_buffer);                                  \
    else                                                                                \
      PASSED();                                                                         \
  } while (0)
#define ASSERT_STRUCT_ARRAY_UNORDERED(__comparator, __size, __expected, __actual, __tag)\
  do {                                                                                  \
    int __idx = array_compare_unordered((int (*) (const void*, const void*, size_t))__comparator,  \
            __expected, __actual, __size, sizeof(__expected[0]));                       \
    if (__idx != -1)                                                                    \
      FAILED("Expecting %s:\n\t%-@\nto be found in array\n\t%@. Called by:\n\t%s",      \
          __tag, 1, &__expected[__idx], __size, __actual, last_call_buffer);            \
    else                                                                                \
      PASSED();                                                                         \
  } while (0)



////////////////////////////////////////////////////////////////
// Asserting that a function has been implemented
//
// ASSERT_IMPLEMENTED(function_name)

#define PASSED() assert_passed()
#define ASSERT_IMPLEMENTED(__fun)                                           \
    do {                                                                    \
        if (((void*)__fun) == NULL || ((void*)__fun) == ((void*)__##__fun)) \
            FAILED("Not implemented: %s", #__fun);                          \
        else                                                                \
            PASSED();                                                       \
    } while(0)

#define ASSERT_IMPLEMENTED_SILENT(__fun)                                      \
    do {                                                                      \
        if (((void*)__fun) == NULL || ((void*)__fun) == ((void*)__##__fun)) { \
            assert_failed();                                                  \
            return;                                                           \
        } else                                                                \
            PASSED();                                                         \
    } while(0)

#define ASSERT_STILL_PADDED(__len, __tab, __tag)  \
  ASSERT_EXACT_MSG(PAD_VALUE, (unsigned char)__tab[__len], "Buffer overflow %sat index %d. Called by:\n\t%s", __tag, __len, last_call_buffer)

// TODO After here macro have to be checked

////////////////////////////////////////////////////////////////
// Asserting if a function is recursive or not
//
// ASSERT_NOT_RECURSIVE(function_name, args)
// ASSERT_RECURSIVE(stack_depth, function_name, args)
//
// Ex. : ASSERT_RECURSIVE(4, fibo_rec, 4) asserts that fibo_rec(4)
// makes 4 recursive calls.

#define TRACE_CALL(__var, __fun, ...)          \
  trace_function(__fun);                       \
  __fun(__VA_ARGS__);                          \
  struct call_info __var = trace_function(NULL)

#define ASSERT_NOT_RECURSIVE(__fun, ...)            \
    do {                                            \
        TRACE_CALL(_call_info, __fun, __VA_ARGS__); \
        ASSERT_EXACT_MSG(1, _call_info.stack_depth, \
            #__fun "() should not be recursive.");  \
    } while(0)
#define ASSERT_RECURSIVE(__stack_depth, __fun, ...)             \
    do {                                                        \
        TRACE_CALL(_call_info, __fun, __VA_ARGS__);             \
        ASSERT_EXACT_MSG(__stack_depth, _call_info.stack_depth, \
            #__fun "() should be recursive.");                  \
    } while(0)

////////////////////////////////////////////////////////////////
// Asserting that a call triggers an assertion or exit() with
// some values.
// These macro should never be nested (nor interleaved).
//
// ASSERT_RAISES(call)
// ASSERT_EXIT_WITH(status, call)

#define ASSERT_RAISES(__call) \
  START_BARRIER(trap)         \
    __call ;                  \
    FAILED(#__call " should raise an assertion."); \
  RESCUE_BARRIER              \
    if (user_has_asserted())  \
      PASSED();        \
    else                      \
      FAILED(#__call " should raise an assertion (not an exit)."); \
  END_BARRIER

#define ASSERT_NO_RAISES(__call) \
  START_BARRIER(trap)            \
    __call ;                     \
    PASSED();             \
  RESCUE_BARRIER                 \
    FAILED(#__call " should not raise an assertion (nor exit)."); \
  END_BARRIER

#define ASSERT_EXIT_WITH(__code, __call)   \
  START_BARRIER(trap)                      \
    __call ;                               \
    FAILED(#__call " should exit with status %d (exit not called).", __code); \
  RESCUE_BARRIER                           \
    trap = (trap > 0)?(trap - 1):(trap+1); \
    if (user_has_asserted())               \
      FAILED(#__call " should exit with status %d not an assertion.", __code); \
    else if (__code != trap)               \
      FAILED(#__call " should exit with status %d not %d.", __code, trap); \
    else                                   \
      PASSED();                     \
  END_BARRIER

////////////////////////////////////////////////////////////////
// Assertions on memory allocation
//
// __BEGIN_MEMORY_CHECKING__
// __END_MEMORY_CHECKING__
//
// Define a block where the memory protection mechanisms are
// enabled. At the end, the memory balance is checked with
// ASSERT_MEMORY_BALANCED.
// Ex. :
// __BEGIN_MEMORY_CHECKING__
//    struct fifo * f = __fifo__empty();
//    // the fifo and the link should have been allocated
//    ASSERT_INT(2, (int) get_a_stats().allocated);
//  __END_MEMORY_CHECKING__

#define BEGIN_MEMORY_CHECKING() ({enable_malloc_protector(); get_a_stats();})
#define END_MEMORY_CHECKING() ({ disable_malloc_protector(); get_a_stats(); })

// ASSERT_MEMORY_BALANCED(checkpoint)
//
// checkpoint must have been called with get_a_stats() before. Then
// the macro compares the memory stats between checkpoint and the
// current point, and tests for memory leaks and invalid free(s).

#define ALLOCATED(__s) \
    ((__s).allocated - (__s).freed)

#define ASSERT_MEMORY(__begin, __end, __expected, __on, ...)        \
    do {                                                            \
        long leaks = ALLOCATED(__end) - ALLOCATED(__begin); \
        long faults = __end.fault - __begin.fault;          \
        int has_failed;                                             \
        if (leaks)                                                  \
            MSG(COLOR(FAILED, "Memory leak")": %ld block%s",        \
                    leaks, leaks == 1 ? "": "s");                   \
        if (faults)                                                 \
            MSG(COLOR(FAILED, "Invalid free")" on %ld pointer%s",   \
                    faults, faults == 1 ? "" : "s");                \
        if ((has_failed = (__expected != __on)))                    \
            MSG(__VA_ARGS__);                                       \
        if (has_failed || leaks || faults) {                        \
            assert_failed();                                        \
            return;                                                 \
        } else                                                      \
            PASSED();                                               \
    } while (0)

////////////////////////////////////////////////////////////////
// FAILED(printf_like_args ...)
//
// Prints an error message to stderr and exits

#ifdef __APPLE__
# define XPRINTF(__file, ...) \
  fxprintf(__file, domain, NULL, __VA_ARGS__)
# define LAST_CALL(...) \
  sxprintf(last_call_buffer, PRINTF_BUFFER_SIZE, domain, NULL, __VA_ARGS__)
  extern printf_domain_t domain;
#else
# define XPRINTF(__file, ...) \
  fprintf(__file, __VA_ARGS__)
# define LAST_CALL(...) \
  snprintf(last_call_buffer, PRINTF_BUFFER_SIZE, __VA_ARGS__)
#endif

#define MSG(...)                                          \
    do {                                                  \
        XPRINTF(stderr, COLOR(WARN, "In %s: "), __func__);\
        XPRINTF(stderr, __VA_ARGS__);                     \
        XPRINTF(stderr, "\n");                            \
        fflush(stderr);                                   \
    } while (0)

#define FAILED(...)       \
    do {                  \
        MSG(__VA_ARGS__); \
        assert_failed();  \
        return;           \
    } while (0)

////////////////////////////////////////////////////////////////
// Define tests cases for a test `test_name`
//
// TEST_CASES(test_name, struct_definition, ...)
// {
//    {case1},
//    {case2},
//    ...
// };
//
// Define tests cases with the same struct_definition than `other_test_name`
// TEST_CASES_LIKE(test_name, other_test_name)
//
// Define an empty test case
// NO_TEST_CASE(test_name)
// You may use UNUSED_TEST_CASES inside your function to mute -Wextra
//
// Reuse the same tests cases than another test. Note: However due to some
// pre-processor limitation, you'll have to use the TEST macro an use
// `other_test_name` as data.
// SAME_TEST_CASES(test_name, other_test_name)
//
// Define a test harness for `test_name`, the function `function_name` should
// be implemented in order to perform all tests
// inside FOR_EACH_TEST, you can use `_` to refer to the test case.
//
// TEST_FUNCTION(test_name) // where test_name == function_name
// TEST_FUNCTION_WITH(test_name, function_name)
// {
//  FOR_EACH_TEST {
//    ... write your test case here
//  }
// }
//
//
// Ex. : (filling an array with even values 0 2 4 6 8 ...)
//
// TEST_CASES(fill_even, int l)
// { { 0 }, { 1 }, { 5 } };
//
// TEST_FUNCTION(fill_even)
// {
//    int expected[6];
//    for (int j = 0; j < ASIZE(expected); j ++)
//      expected[j] = 2 * j;
//    FOR_EACH_TEST {
//        int result[6];
//
//        PAD_BUFFER(result);
//        DESCRIBE(fill_even(_.l, result),
//                "fill_even(%d, result)", _.l);
//
//        ASSERT_ARRAY(_.l, expected, result, "%d", AS_PARAM(result);
//        ASSERT_STILL_PADDED(_.l + 1, result, AS_PARAM(result));
//    }
//  }

#define _ tests_values[test_idx]

#define FOR_EACH_TEST \
  for (int test_idx = 0; test_idx < tests_count ; test_idx++) \
    if (test_each_setup(), 1)

#define TEST_CASES(__name, ...)           \
  typedef struct {                        \
      FOR_EACH(X_TC_ENTRY, , __VA_ARGS__) \
  } tc_##__name;                          \
  tc_##__name test_ ## __name ## _values[] =

#define NO_TEST_CASE(__name)                \
    TEST_CASES(__name, int unused) {{0}}

#define SAME_TEST_CASES(__name, __other)  \
  typedef tc_##__other tc_##__name

#define TEST_CASES_LIKE(__name, __other)  \
  typedef tc_##__other tc_##__name;       \
  tc_##__name test_##__name##_values[] =

#define X_TC_ENTRY(a, v) v;


#define TEST_FUNCTION(...) TEST_FUNCTION_RELAY(__VA_ARGS__)
#define TEST_FUNCTION_WITH(...) TEST_FUNCTION_WITH_RELAY(__VA_ARGS__)

#define TEST_FUNCTION_RELAY(__name, ...) TEST_FUNCTION_WRAPPER(__name, __name, __VA_ARGS__)
#define TEST_FUNCTION_WITH_RELAY(__name, __fun, ...) TEST_FUNCTION_WRAPPER(__name, __fun, __VA_ARGS__)

#define GLOBAL_SETUP \
    static void __attribute__((constructor)) global_setup()
#define LOCAL_SETUP(...) \
    void test_local_setup(__VA_ARGS__)
#define TEST_SETUP(...) \
    void test_each_setup(__VA_ARGS__)

void test_each_setup();
void test_local_setup();

#define CALL_LOCAL_TEST_SETUP(f, ...) \
      test_local_setup(__VA_ARGS__)

#define TESTED_FUNCTION(__fun, ...)                                 \
    do {                                                                      \
        if (((void*)__fun) == NULL || ((void*)__fun) == ((void*)__##__fun)) { \
            assert_failed();                                                  \
            return;                                                           \
        } else                                                                \
            PASSED();                                                         \
    } while(0)


#define TEST_FUNCTION_WRAPPER(__name, ...)                                          \
  ; /* this UGLY semi-colon is here to avoid the forgotten ones after test cases */ \
  static void test_##__name(int tests_count, tc_##__name tests_values[]);           \
  static void test_##__name##_wrapper(int tests_count, tc_##__name tests_values[]) {\
      TESTED_FUNCTION(__VA_ARGS__);                                                 \
      CALL_LOCAL_TEST_SETUP(__VA_ARGS__);                                           \
      test_##__name(tests_count, tests_values);                                     \
  }                                                                                 \
  static void test_##__name(int tests_count, tc_##__name tests_values[])

#define UNUSED_TEST_CASES       \
  do { (void) tests_values;     \
  (void) tests_count;           \
  test_each_setup();            \
  } while (0)

////////////////////////////////////////////////////////////////
// Buffers helpers
// Pad a buffer with repeated 0x5a, up to the last element which is padded by 0
//
// PAD_BUFFER(buffer) // Buffer should be an array
// PAD_DYNAMIC_BUFFER(buffer, size) // Buffer could be anything dereferenceable
#define PAD_VALUE 0x5a
#define PAD_BUFFER(__buffer)                                \
    memset(__buffer, PAD_VALUE, sizeof(__buffer) - 1);      \
    (__buffer)[sizeof(__buffer)/sizeof(__buffer[0]) - 1] = 0
#define PAD_DYNAMIC_BUFFER(__buffer, __len)                 \
    memset(__buffer, PAD_VALUE, (__len * buffer[0])- 1);    \
    (__buffer)[__len - 1] = 0

////////////////////////////////////////////////////////////////
// PRINT_RENDERER(struct_name,var_name,string,fields...)
// Configures a printer for a given struct name.
// The var_name must be the one used inside the fields.
//
// Ex. :
//
// PRINT_RENDERER(iris, i, "{ %s, %d, %d, %d, %d }",
//                iris_gender[i->gender],
//                i->sepal_length, i->sepal_width,
//                i->petal_length, i->petal_width);
//
// Then the renderer can be used with something like :
//
// ASSERT_STRING_MSG(expected, actual, "print_iris(%-@)", 1, &exemples[i]);
//
// The "1" is the size of the array to display. The `-` disable the `{}` around
// items. Some exemples are given in 'iris' and 'planning'.

struct print_renderer {
  size_t type_size;
  const char *(*get_format)(const struct printf_info * info);
  int (*render)(FILE *stream, const char *fmt, const void * item);
};

void struct_renderer(const struct print_renderer render);
void struct_renderer_alt(const struct print_renderer render);

#define PRINT_RENDERER(__struct, __var, __patern, ...)                        \
  static const char* array_##__struct ##_fmt(const struct printf_info *info) {\
    return info->showsign ? ",\n"__patern : ", "__patern;                     \
  }                                                                           \
  static int array_##__struct(                                                \
          FILE *stream, const char *fmt, const struct __struct * __var) {     \
    return fprintf(stream, fmt, __VA_ARGS__);                                 \
  }                                                                           \
  static const struct print_renderer render_ ## __struct = {                  \
      sizeof(struct __struct), array_##__struct##_fmt,                        \
      (int (*)(FILE *, const char* , const void *))array_##__struct           \
  }

int printf_print_array(FILE *stream, const struct print_renderer *renderer,
        const struct printf_info *info, const void *const *args);

// See cocktail/exercice.c for exemple of custom renderer

#define CUSTOM_PRINT_RENDERER(__struct, __name)                               \
  static const char* array_##__struct##_fmt(const struct printf_info *info) { \
    (void)info;                                                               \
    return ", ";                                                              \
  }                                                                           \
  static int __name(                                                          \
          FILE *, const char *, const struct __struct *) ;                    \
  static const struct print_renderer render_ ## __struct = {                  \
      sizeof(struct __struct), array_##__struct##_fmt,                        \
      (int (*)(FILE *, const char* , const void *))__name                     \
  }

#define CALL_PRINT_RENDERER(__len, __renderer, __count, __items)            \
    do {                                                                    \
      int size = __count;                                                   \
      const void *items = __items;                                          \
      const void *const print_args[] = {&size, &items};                     \
      __len += printf_print_array(stream, &__renderer, &info, print_args);  \
    } while (0)


#define DEFINE_STRUCT_COMPARATOR(__struct)                    \
  static int compare_##__struct(                              \
      const struct __struct *p1, const struct __struct *p2) { \
    return !memcmp(p1, p2, sizeof(struct __struct));          \
  }

////////////////////////////////////////////////////////////////
// Builds a test suite containing all the tests in the list and
// execute them one by one.
//
// EXERCICE(prefix, list_of_tests ...)
// is an equivalent of
// BEGIN_TESTS_LIST
//  TEST(prefix ":" test1, test1 (function), test1 (data)) 
//  TEST(prefix ":" test2, test2 (function), test2 (data)) 
// END_TESTS_LIST
//
// if SAME_TEST_CASES is used it can only be used with the later form

#define TEST(__name, __fun, __data)                         \
  { __name, (testfun_t)test_##__fun##_wrapper,              \
    ASIZE(test_##__data##_values), &test_##__data##_values},

#define TESTS(__name, ...)                \
    FOR_EACH(X_TEST, __name, __VA_ARGS__)

#define EXERCICE(__name, ...)   \
  BEGIN_TESTS_LIST              \
    TESTS(__name, __VA_ARGS__)  \
  END_TESTS_LIST

#define BEGIN_TESTS_LIST            \
    const struct test exercice[] = {
#define END_TESTS_LIST              \
          { .name= NULL}            \
    };

#define UNUSED_TESTS(...)                   \
    const struct test unused_exercice[] = { \
        TESTS("unused", __VA_ARGS__)        \
    };

#define X_TEST(x, n) TEST(x":"#n, n, n)

////////////////////////////////////////////////////////////////
// PROVIDED_FUNCTION(return_type, function_name, [args_types ...])
//
// Defines a function named function_name that can be used within the
// code and the tests. The goal is for the student to be able to use
// the function even if he has not implemented it. If not implemented,
// the weak function will return false with ASSERT_IMPLEMENTED. To use
// the function in the tests, it suffices to prefix it with '__'.

#ifdef __APPLE__
#   ifdef ANSWER
#   define PROVIDED_FUNCTION(__name, __ret, ...) \
      extern __ret __name(__VA_ARGS__); \
      static __ret __##__name(__VA_ARGS__)
#   else
#   define PROVIDED_FUNCTION(__name, __ret, ...) \
      static __ret __##__name(__VA_ARGS__) {exit(1);} \
      static __ret __name(__VA_ARGS__)
#   endif
#else
#   define PROVIDED_FUNCTION(__ret, __name, ...) \
    extern __ret __name(__VA_ARGS__) \
    __attribute__ ((weak, alias(WEAK_DEFINE_RELAY(__##__name)))); \
    static __ret __##__name(__VA_ARGS__)
#   define WEAK_DEFINE_RELAY(__x) #__x
#endif

////////////////////////////////////////////////////////////////
// Public interface

// Test function
typedef int (* const testfun_t) (int, const void*);

// Struct for the tests
struct test {
  const char *name;     // test name
  const testfun_t fun;  // test function
  int len;              // length of the parameters
  const void *param;    // parameters
};

// Call statistics
struct call_info {
  unsigned count;
  unsigned stack_depth;
};

// Allocation statistics
struct a_stats  {
  unsigned long allocated, freed, reallocated;
  unsigned long fault;
};

// Get allocation statistics
struct a_stats get_a_stats();
void enable_malloc_protector();
void disable_malloc_protector();
void free_all();
size_t pointer_info(void*);

// Get call info (for recursion etc..)
struct call_info trace_function(void *sym);

// Main entry point
int grader(const struct test tests[], int argc, char **argv);
void list_tests(const struct test tests[]);

struct printer {
  void (*header)();
  void (*result)(const struct test*, int);
  void (*summary)();
};

const struct test *find_test(const struct test test[], const char *name);
int run_test(const struct test *test, const struct printer *print);
int run_test_group(const struct test test[], const struct printer *print);
void assert_passed();
void assert_failed();

// User assert support
#define START_BARRIER(__trap)  \
    do {                       \
      int __trap;              \
      reset_user_assert();     \
      jmp_buf _barrier;        \
      if (!(__trap = setjmp(_barrier))) { \
        register_barrier(_barrier);

#define RESCUE_BARRIER         \
      } else {

#define END_BARRIER            \
      }                        \
      release_barrier();       \
    } while(0);

int user_has_asserted();
void reset_user_assert();
void release_barrier();
void register_barrier(jmp_buf checkpoint);

// Grab printf feature
// Note: the size of the buffer is controled by
// -DPRINTF_BUFFER_SIZE=4096
// which has to be set while compiling grader.c (in libexam)
void start_grab_printf();
char* end_grab_printf();

enum grader_mode_t { MAIN, TEST, GRADE };
enum grader_mode_t get_grader_mode();

// Renderers
//void print_test_result(const struct test *test, int status);
//void print_raw_test_result(const struct test *test, int status);

// Helpers
int normalize(int v); // force positive values to 1 and negative to -1.
int as_bool(int v); // force true to 1.

size_t strnlen(const char *s, size_t maxlen);

int compare_memory(const void* v1, const void* v2, size_t size);

typedef int (*comparator)(const void*, const void*);

#define DO_SEGFAULT() do { int *v = NULL; *v = 1; } while(0)

// Compare two arrays using comparator, and return the differing index
// or -1 if none.
int array_compare(int (*comparator) (const void*, const void*, size_t),
        const void *a1, const void *a2, size_t n, size_t item_size);

// Compare two arrays using comparator, and return the differing index
// or -1 if none. Order of items does not matters
int array_compare_unordered(comparator comparator,
    const void *a1, const void *a2, size_t n, size_t item_size);

extern char last_call_buffer[PRINTF_BUFFER_SIZE];

// Preprocessor helpers
#define ASIZE(__sym) (sizeof(__sym)/sizeof(__sym[0]))

#define FE_1(WHAT, X, Y) WHAT(X, Y)
#define FE_2(WHAT, X, Y, ...) WHAT(X, Y) FE_1(WHAT, X, __VA_ARGS__)
#define FE_3(WHAT, X, Y, ...) WHAT(X, Y) FE_2(WHAT, X, __VA_ARGS__)
#define FE_4(WHAT, X, Y, ...) WHAT(X, Y) FE_3(WHAT, X, __VA_ARGS__)
#define FE_5(WHAT, X, Y, ...) WHAT(X, Y) FE_4(WHAT, X, __VA_ARGS__)
#define FE_6(WHAT, X, Y, ...) WHAT(X, Y) FE_5(WHAT, X, __VA_ARGS__)
#define FE_7(WHAT, X, Y, ...) WHAT(X, Y) FE_6(WHAT, X, __VA_ARGS__)
#define FE_8(WHAT, X, Y, ...) WHAT(X, Y) FE_7(WHAT, X, __VA_ARGS__)
#define FE_9(WHAT, X, Y, ...) WHAT(X, Y) FE_8(WHAT, X, __VA_ARGS__)
#define FE_10(WHAT, X, Y, ...) WHAT(X, Y) FE_9(WHAT, X, __VA_ARGS__)
#define FE_11(WHAT, X, Y, ...) WHAT(X, Y) FE_10(WHAT, X, __VA_ARGS__)
#define FE_12(WHAT, X, Y, ...) WHAT(X, Y) FE_11(WHAT, X, __VA_ARGS__)
#define FE_13(WHAT, X, Y, ...) WHAT(X, Y) FE_12(WHAT, X, __VA_ARGS__)
#define FE_14(WHAT, X, Y, ...) WHAT(X, Y) FE_13(WHAT, X, __VA_ARGS__)
#define FE_15(WHAT, X, Y, ...) WHAT(X, Y) FE_14(WHAT, X, __VA_ARGS__)
#define FE_16(WHAT, X, Y, ...) WHAT(X, Y) FE_15(WHAT, X, __VA_ARGS__)
#define FE_17(WHAT, X, Y, ...) WHAT(X, Y) FE_16(WHAT, X, __VA_ARGS__)
#define FE_18(WHAT, X, Y, ...) WHAT(X, Y) FE_17(WHAT, X, __VA_ARGS__)
#define FE_19(WHAT, X, Y, ...) WHAT(X, Y) FE_18(WHAT, X, __VA_ARGS__)
#define FE_20(WHAT, X, Y, ...) WHAT(X, Y) FE_19(WHAT, X, __VA_ARGS__)
#define FE_21(WHAT, X, Y, ...) WHAT(X, Y) FE_20(WHAT, X, __VA_ARGS__)
#define FE_22(WHAT, X, Y, ...) WHAT(X, Y) FE_21(WHAT, X, __VA_ARGS__)
#define FE_23(WHAT, X, Y, ...) WHAT(X, Y) FE_22(WHAT, X, __VA_ARGS__)
#define FE_24(WHAT, X, Y, ...) WHAT(X, Y) FE_23(WHAT, X, __VA_ARGS__)
#define FE_25(WHAT, X, Y, ...) WHAT(X, Y) FE_24(WHAT, X, __VA_ARGS__)
#define FE_26(WHAT, X, Y, ...) WHAT(X, Y) FE_25(WHAT, X, __VA_ARGS__)
#define FE_27(WHAT, X, Y, ...) WHAT(X, Y) FE_26(WHAT, X, __VA_ARGS__)
#define FE_28(WHAT, X, Y, ...) WHAT(X, Y) FE_27(WHAT, X, __VA_ARGS__)
#define FE_29(WHAT, X, Y, ...) WHAT(X, Y) FE_28(WHAT, X, __VA_ARGS__)
#define FE_30(WHAT, X, Y, ...) WHAT(X, Y) FE_29(WHAT, X, __VA_ARGS__)

#define GET_MACRO(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,\
                 _20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,NAME,...) NAME
#define FOR_EACH(action, arg, ...) \
    GET_MACRO(__VA_ARGS__,\
        FE_30,FE_29,FE_28,FE_27,FE_26,FE_25,FE_24,FE_23,FE_22,FE_21,\
        FE_20,FE_19,FE_18,FE_17,FE_16,FE_15,FE_14,FE_13,FE_12,FE_11,\
        FE_10,FE_9,FE_8,FE_7,FE_6,FE_5,FE_4,FE_3,FE_2,FE_1)(action, arg, __VA_ARGS__)

#define PP_NARG(...) \
        PP_NARG_(__VA_ARGS__,PP_RSEQ_N())
#define PP_NARG_(...) \
              PP_ARG_N(__VA_ARGS__)
#define PP_ARG_N( \
         _1, _2, _3, _4, _5, _6, _7, _8, _9,_10, \
        _11,_12,_13,_14,_15,_16,_17,_18,_19,_20, \
        _21,_22,_23,_24,_25,_26,_27,_28,_29,_30, \
        _31,_32,_33,_34,_35,_36,_37,_38,_39,_40, \
        _41,_42,_43,_44,_45,_46,_47,_48,_49,_50, \
        _51,_52,_53,_54,_55,_56,_57,_58,_59,_60, \
        _61,_62,_63,  N, ...) N
#define PP_RSEQ_N() \
              63,62,61,60,                   \
            59,58,57,56,55,54,53,52,51,50, \
            49,48,47,46,45,44,43,42,41,40, \
            39,38,37,36,35,34,33,32,31,30, \
            29,28,27,26,25,24,23,22,21,20, \
            19,18,17,16,15,14,13,12,11,10, \
             9, 8, 7, 6, 5, 4, 3, 2, 1, 0


#endif // __GRADER_H
