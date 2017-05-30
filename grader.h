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
#ifndef STACK_LIMIT
# define STACK_LIMIT (1 * 1024 * 1024)
#endif

#define DESCRIBE(__call, ...)\
  ({ LAST_CALL(__VA_ARGS__); __call ;})


////////////////////////////////////////////////////////////////
// Assertions
////////////////////////////////////////////////////////////////
// Asserting x == y
//
// ASSERT_EXACT    (format,  expected_val, actual_val)
// ASSERT_EXACT_MSG(format,  message, expected_val, actual_val)
// ASSERT_MSG      (message, expected_val, actual_val)
//
// format  is the formatting string used to print the values
// message is the string that will be printed in front of the message
// ASSERT_MSG just prints the message and not the values

#	define ASSERT_EXACT(__fmt, __expected, __actual)                  \
    ASSERT_EXACT_MSG(__fmt, #__actual, __expected, __actual)
#undef ASSERT_EXACT
#	define ASSERT_EXACT(__fmt, __expected, __actual)                  \
	do {                                                              \
		if ((__expected) != (__actual))                                 \
			FAILED("%s: expecting '"__fmt"', but found '"__fmt"'", last_call_buffer, __expected, __actual);\
		else                                                          \
			ASSERT_PASSED();                                          \
	} while (0)

#	define ASSERT_EXACT_MSG(__fmt, __expr, __expected, __actual)      \
	do {                                                              \
		if ((__expected) != (__actual))                                 \
			FAILED(__expr ": expecting '"__fmt"', but found '"__fmt"'", __expected, __actual);\
		else                                                          \
			ASSERT_PASSED();                                          \
	} while (0)

#	define ASSERT_EXACT_DESCRIBED(__fmt, __expected, __actual, __args_fmt, ...)  \
	do {                                                                \
		if ((__expected) != (__actual))                                 \
			FAILED("Expecting '"__fmt"', but found '"__fmt"'.\nCalled by: "__args_fmt, __expected, __actual, __VA_ARGS__);\
		else                                                             \
			ASSERT_PASSED();                                             \
	} while (0)

#	define ASSERT_MSG(__msg, __expected, __actual)              \
	do {                                                        \
		if ((__expected) != (__actual)) {                       \
			FAILED(__msg);                                      \
        } else {                                                \
			ASSERT_PASSED();                                    \
        }                                                       \
	} while (0)

////////////////////////////////////////////////////////////////
// Asserting x~= y
//
// ASSERT_FP(format, expected_val, actual_val, precision)
//
// assertion for floating point values, asserting equality up to a
// precision in absolute value
#	define ASSERT_FP(__fmt, __expected, __actual, __prec)		\
	do {                                                                         \
		if (fabs((__expected) - (__actual)) > (__prec))                      \
			FAILED("Expecting '"__fmt"', but found '"__fmt"'", __expected, __actual);\
		else                                                                 \
			ASSERT_PASSED();                                             \
	} while (0)

////////////////////////////////////////////////////////////////
// Dedicated assertions for specific types
#	define ASSERT_INT(__expected, __actual) \
	ASSERT_EXACT("%d", __expected, __actual)
#	define ASSERT_INT_MSG(__expected, __actual, __argsfmt, ...) \
	ASSERT_EXACT_DESCRIBED("%d", __expected, __actual, __argsfmt, __VA_ARGS__)
#	define ASSERT_UINT(__expected, __actual) \
	ASSERT_EXACT("%u", __expected, __actual)
#	define ASSERT_UINT_MSG(__expected, __actual, __argsfmt, ...) \
	ASSERT_EXACT_DESCRIBED("%u", __expected, __actual, __argsfmt, __VA_ARGS__)
#	define ASSERT_LONG(__expected, __actual) \
	ASSERT_EXACT("%ld", __expected, __actual)
#	define ASSERT_ULONG(__expected, __actual) \
	ASSERT_EXACT("%lu", __expected, __actual)
#	define ASSERT_FLOAT(__expected, __actual, __prec) \
	ASSERT_FP("%f", __expected, __actual, __prec)
#	define ASSERT_DOUBLE(__expected, __actual, __prec) \
	ASSERT_FP("%lf", __expected, __actual, __prec)

#	define ASSERT_PTR(__expected, __actual) \
	ASSERT_EXACT("%p", __expected, __actual)
#	define ASSERT_PTR_MSG(__expected, __actual, __argsfmt, ...) \
	ASSERT_EXACT_DESCRIBED("%p", __expected, __actual, __argsfmt, __VA_ARGS__)

#	define ASSERT_NULL(__actual) \
	ASSERT_EXACT("%p", NULL, __actual)
#	define ASSERT_NULL_MSG(__actual, __argsfmt, ...) \
	ASSERT_EXACT_DESCRIBED("%p", NULL, __actual, __argsfmt, __VA_ARGS__)

#	define ASSERT_STRUCT(__struct_comparator, __expected, __actual) \
	do {                                                                      \
		if (!(((__expected) == (__actual)) || (__struct_comparator((__expected), (__actual)))))\
			FAILED("Expecting: %-@ but found: %-@\nCalled by: %s", 1, (__expected), 1, (__actual), last_call_buffer);\
		else                                                             \
			ASSERT_PASSED();                                             \
	} while (0)
#	define ASSERT_STRUCT_MSG(__struct_comparator, __expected, __actual, __args_fmt, ...) \
	do {                                                                      \
		if (!(((__expected) == (__actual)) || (__struct_comparator((__expected), (__actual)))))\
			FAILED("Expecting: %-@ but found: %-@\nCalled by: "__args_fmt, 1, (__expected), 1, (__actual), __VA_ARGS__);\
		else                                                             \
			ASSERT_PASSED();                                             \
	} while (0)

# define ASSERT_ARRAY_MSG(__size, __expected, __actual, __fmt, __args_fmt, ...) \
  do { \
    int __idx = array_compare(compare_memory, __expected, __actual, __size, sizeof(__expected[0])); \
    if (__idx != -1) \
      FAILED("Expecting\n"__fmt" but found\n"__fmt" at index %d.\nCalled by: "__args_fmt, \
          __expected[__idx], __actual[__idx], __idx, __VA_ARGS__);\
    else \
      ASSERT_PASSED(); \
  } while (0)

# define ASSERT_ARRAY_STRUCT_MSG(__size, __expected, __actual, __fmt, __args_fmt, ...) \
  do { \
    int __idx = array_compare(compare_memory, __expected, __actual, __size, sizeof(__expected[0])); \
    if (__idx != -1) \
      FAILED("Expecting: "__fmt" but found: "__fmt" at index %d.\nCalled by: "__args_fmt, \
          1, &__expected[__idx], 1, &__actual[__idx], __idx, __VA_ARGS__);\
    else \
      ASSERT_PASSED(); \
  } while (0)

#   define ASSERT_NOT_NULL(__actual) \
	do {                                                                         \
		if (NULL == (__actual))                                      \
			FAILED("'"#__actual"', expecting not NULL");\
		else                                                                 \
			ASSERT_PASSED();                                             \
	} while (0)

#   define ASSERT_NOT_NULL_MSG(__actual, __args_fmt, ...)			\
	do {                                                                         \
		if (NULL == (__actual))                                      \
		  FAILED("'"#__actual"', expecting not NULL: "__args_fmt, __VA_ARGS__); \
		else                                                                 \
			ASSERT_PASSED();                                             \
	} while (0)

#   define ASSERT_BOOL(__expected, __actual) \
    ASSERT_EXACT_MSG("%d", #__actual, ((__expected) == 0), ((__actual) == 0))

#   define ASSERT_BOOL_MSG(__expected, __actual, __argsfmt, ...) \
  ASSERT_EXACT_DESCRIBED("%d", ((__expected) == 0), ((__actual) == 0), __argsfmt,  __VA_ARGS__)

#   define ASSERT_TRUE(__actual)		\
        ASSERT_BOOL(1, __actual)

#   define ASSERT_TRUE_MSG(__actual, __argsfmt, ...)	\
  ASSERT_BOOL_MSG(1, __actual, __argsfmt, __VA_ARGS__)

#   define ASSERT_FALSE(__actual)		\
        ASSERT_BOOL(0, __actual)

#   define ASSERT_FALSE_MSG(__actual, __argsfmt, ...)	\
  ASSERT_BOOL_MSG(0, __actual, __argsfmt, __VA_ARGS__)

#	define ASSERT_STRING(__expected, __actual)			\
	do {                                                               \
		if (memcmp(__expected, __actual, strlen(__expected) + 1))        \
			FAILED("Expecting\n'%s', but found\n'%s'", __expected, __actual);\
		else                                                       \
			ASSERT_PASSED();                                   \
	} while (0)
#	define ASSERT_STRING_MSG(__expected, __actual, __args_fmt, ...) \
	do {                                                               \
		if (memcmp(__expected, __actual, strlen(__expected) + 1))  \
			FAILED("Expecting\n'%s', but found\n'%s'.\nCalled by: "__args_fmt, __expected, __actual, __VA_ARGS__);\
		else                                                       \
			ASSERT_PASSED();                                   \
	} while (0)
#	define ASSERT_CHAR(__expected, __actual)                         \
	do {                                                               \
		if (__expected != __actual)        \
		  FAILED("Expecting '%c' (code %hhd), but found '%c' (code %hhd)", __expected, __expected, __actual, __actual); \
		else                                                        \
			ASSERT_PASSED();                                    \
	} while (0)

#	define ASSERT_VALID_PTR(__actual)			\
	do {                                                               \
		if (invalid_stack_pointer(__actual))        \
			FAILED("Expecting valid pointer, but pointer (%p) is above in stack.", __actual);\
		else                                                       \
			ASSERT_PASSED();                                   \
	} while (0)
#	define ASSERT_VALID_PTR_MSG(__actual, __args_fmt, ...) \
	do {                                                               \
		if (invalid_stack_pointer(__actual))        \
			FAILED("Expecting valid pointer, but pointer (%p) is above in stack.\ Called by: "__args_fmt, __actual, __VA_ARGS__);\
		else                                                       \
			ASSERT_PASSED();                                   \
	} while (0)

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
        ASSERT_MSG(#__fun "() should not be recursive.", 1, _call_info.stack_depth); \
    } while(0)
#define ASSERT_RECURSIVE(__stack_depth, __fun, ...) \
    do {                                            \
        TRACE_CALL(_call_info, __fun, __VA_ARGS__); \
        ASSERT_MSG(#__fun "() should be recursive.", __stack_depth, _call_info.stack_depth); \
    } while(0)

////////////////////////////////////////////////////////////////
// Asserting that a function has been implemented
//
// ASSERT_IMPLEMENTED(function_name)

#define ASSERT_PASSED() assert_passed()
#define ASSERT_IMPLEMENTED(__fun)				                    \
	do {                                                                        \
		if (((void*)__fun) == NULL || ((void*)__fun) == ((void*)__##__fun)) \
			FAILED("Not implemented: %s", #__fun);                      \
		else                                                                \
			ASSERT_PASSED();                                            \
	} while(0)

#define ASSERT_IMPLEMENTED_SILENT(__fun)		\
	do {                                      \
		if (((void*)__fun) == NULL || ((void*)__fun) == ((void*)__##__fun)) {\
      assert_failed();                      \
      return EXIT_FAILURE;                  \
    } else                                  \
			ASSERT_PASSED();                      \
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
      ASSERT_PASSED();        \
    else                      \
      FAILED(#__call " should raise an assertion (not an exit)."); \
  END_BARRIER

#define ASSERT_NO_RAISES(__call) \
  START_BARRIER(trap)            \
    __call ;                     \
    ASSERT_PASSED();             \
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
      ASSERT_PASSED();                     \
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

#define __BEGIN_MEMORY_CHECKING__               \
    do {                                        \
        struct a_stats _m_ck_pt = get_a_stats();\
        enable_malloc_protector();              \

#define __END_MEMORY_CHECKING__                 \
        disable_malloc_protector();             \
        ASSERT_MEMORY_BALANCED(_m_ck_pt);       \
    }while(0);

// ASSERT_MEMORY_BALANCED(checkpoint)
//
// checkpoint must have been called with get_a_stats() before. Then
// the macro compares the memory stats between checkpoint and the
// current point, and tests for memory leaks and invalid free(s).

#define ALLOCATED(__s) \
    ((__s).allocated - (__s).freed)
#define ASSERT_MEMORY_BALANCED(__saved_stats)                   \
	do {                                                    \
        struct a_stats __stats = get_a_stats();                 \
        long leaks = ALLOCATED(__stats) - ALLOCATED(__saved_stats); \
        long faults = __stats.fault - __saved_stats.fault;      \
        if (leaks)                                              \
            FAILED(COLOR(FAILED, "Memory leak")": %ld block%s", \
                    leaks, leaks == 1 ? "": "s");               \
        if (faults)                                             \
            FAILED(COLOR(FAILED, "Invalid free")" on %ld pointer%s", \
                    faults, faults == 1 ? "" : "s");            \
        if (leaks == 0 && faults == 0)                          \
            ASSERT_PASSED();                                    \
	} while (0)

// FREE_ALL
//
// Calls free_all() to free all pointers handled by the malloc
// library. Warning : this does not work for all malloc handlers.

#define FREE_ALL()                  \
    do {                            \
        enable_malloc_protector();  \
        free_all();                 \
        disable_malloc_protector(); \
    } while(0)

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

#define MSG(...)                                      \
	do {                                                \
		XPRINTF(stderr, COLOR(WARN, "In %s: "), __func__);\
		XPRINTF(stderr, __VA_ARGS__);                     \
		XPRINTF(stderr, "\n");                            \
    fflush(stderr);                                   \
	} while (0)

#define FAILED(...)     \
	do {                  \
    MSG(__VA_ARGS__);   \
    assert_failed();    \
		return EXIT_FAILURE;\
	} while (0)

////////////////////////////////////////////////////////////////
// Test macro helpers
//
// TEST(message, function_name, function_args ...)
//
// Generate a struct test with a better syntax.
// Ex. : TEST("allocating a fifo", test_allocate_fifo, NULL),

#define ASIZE(__sym) (sizeof(__sym)/sizeof(__sym[0]))
#define TEST(__name, __fun, __data)			\
  {__name, (testfun_t)__fun, ASIZE(__data), &__data}

////////////////////////////////////////////////////////////////
// TEST_CASES(test_name, struct_definition, list_of_cases ...)
// TEST_FUNCTION(function_name) // where test_name == function_name
// TEST_FUNCTION_WITH(test_name, function_name)
// END_TEST_FUNCTION
//
// The first macro defins a list of test cases for the function that
// will be defined within the block TEST_FUNCTION
// ... END_TEST_FUNCTION. Within this block, one can access 'len' the
// number of test cases and values[] the test cases array.
//
// Ex. : (filling an array with even values 0 2 4 6 8 ...)
//
// TEST_CASES(fill_even, { int l ;})
//  { 0 }, { 1 }, { 5 }
// END_TEST_CASES
// TEST_FUNCTION(fill_even)
//        for (int i = 0 ; i < len ; i++) {
//        int result[] = { -1, -1, -1, -1, -1, -1};
//        int l = values[i].l;
//        fill_even(l, result);
//        for (int j = 0; j < l; j ++) {
//            ASSERT_INT(2 * j, result[j]);
//        }
//    }
// END_TEST_FUNCTION
//
// In order to test some code but with another test name, use the
// TEST_FUNCTION_WITH macro. In fact, every test should refer to some
// code written by the students. For example :
//
// TEST_FUNCTION_WITH(fill_even_boundary_check, fill_even)
//   ...
// END_TEST_FUNCTION

#define TEST_CASES(__name, __type) \
    struct __name __type test_ ## __name ## _values[] = {
#define END_TEST_CASES \
    };
#define NO_TEST_CASE(__name) \
      struct __name {int no;} test_ ## __name ## _values[] = {{ 0 }};

#define TEST_FUNCTION_GENERIC(__name) \
  int test_ ## __name (int len, struct __name values[]) { \

#define TEST_FUNCTION(__name) \
  TEST_FUNCTION_GENERIC(__name) \
	ASSERT_IMPLEMENTED_SILENT(__name);

#define TEST_FUNCTION_WITH(__name, __fun)	          \
  TEST_FUNCTION_GENERIC(__name) \
	ASSERT_IMPLEMENTED_SILENT(__fun);

#define END_TEST_FUNCTION \
	return EXIT_SUCCESS;    \
  }

// PAD_BUFFER(buffer, size)
#define PAD_BUFFER(__buffer)                     \
    memset(__buffer, 0x5a, ASIZE(__buffer) - 1); \
    (__buffer)[ASIZE(__buffer) - 1] = 0

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

#define PRINT_RENDERER(__struct, __var, __patern, ...) \
  static const char* array_##__struct ##_fmt(const struct printf_info *info) { \
    return ", "__patern; \
  } \
  static int array_##__struct(FILE *stream, const char *fmt, const struct __struct * __var) { \
    return fprintf(stream, fmt, __VA_ARGS__); \
  } \
  static const struct print_renderer render_ ## __struct = {sizeof(struct __struct), array_##__struct##_fmt, (int (*)(FILE *, const char* , const void *))array_##__struct}


#define DEFINE_STRUCT_COMPARATOR(__struct)                    \
  static int compare_##__struct(                              \
      const struct __struct *p1, const struct __struct *p2) { \
    return !memcmp(p1, p2, sizeof(struct point_t));    \
  }

////////////////////////////////////////////////////////////////
// EXERCICE(message, list_of_test_function_names ...)
//
// Builds a test suite containing all the tests in the list and
// execute them one by one.

#define TNAME(x, n) TEST(x ":" #n, test_ ## n, test_ ## n ## _values),

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

#define EXERCICE(_name, ...)	      \
    const struct test exercice [] = { \
          FOR_EACH(TNAME, _name, __VA_ARGS__) \
          { .name= NULL} \
    };

////////////////////////////////////////////////////////////////
// WEAK_DEFINE(function_name, return_type, args_types ...) { block }
//
// Defines a function named function_name that can be used within the
// code and the tests. The goal is for the student to be able to use
// the function even if he has not implemented it. If not implemented,
// the weak function will return false with ASSERT_IMPLEMENTED. To use
// the function in the tests, it suffices to prefix it with '__'.

#define WEAK_DEFINE_RELAY(__x) #__x
#ifdef __APPLE__
#   ifdef ANSWER
#   define PROVIDED_FUNCTION(__name, __ret, ...) \
      extern __ret __name(__VA_ARGS__); \
      static __ret __##__name(__VA_ARGS__) {
#   define WEAK_DEFINE(__name, __ret, ...) \
      extern __ret __name(__VA_ARGS__); \
      static __ret __##__name(__VA_ARGS__)
#   else
#   define PROVIDED_FUNCTION(__name, __ret, ...) \
      static __ret __##__name(__VA_ARGS__) {exit(1);} \
      static __ret __name(__VA_ARGS__) {
#   define WEAK_DEFINE(__name, __ret, ...) \
      static __ret __##__name(__VA_ARGS__) {exit(1);} \
      static __ret __name(__VA_ARGS__)
#   endif
#else
#   define PROVIDED_FUNCTION(__name, __ret, ...) \
 	extern __ret __name(__VA_ARGS__) \
 	__attribute__ ((weak, alias(WEAK_DEFINE_RELAY(__##__name)))); \
 	static __ret __##__name(__VA_ARGS__) {
#   define WEAK_DEFINE(__name, __ret, ...) \
 	extern __ret __name(__VA_ARGS__) \
 	__attribute__ ((weak, alias(WEAK_DEFINE_RELAY(__##__name)))); \
 	static __ret __##__name(__VA_ARGS__)
#endif
#define END_PROVIDED_FUNCTION }

////////////////////////////////////////////////////////////////
// End of macros
////////////////////////////////////////////////////////////////

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
// !! Remark that END_BARRIER should be followed by a semi-column, while not the others
//
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
    } while(0)

int user_has_asserted();
void reset_user_assert();
void release_barrier();
void register_barrier(jmp_buf checkpoint);

int invalid_stack_pointer(const void *sentinel_addr);

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
int compare_memory(const void* v1, const void* v2, size_t size);

// Compare two arrays and return the differing index or -1 if none
int array_compare(int (*comparator)(const void*, const void*, size_t),
        const void *a1, const void *a2, size_t n, size_t item_size);

extern char last_call_buffer[PRINTF_BUFFER_SIZE];
#endif // __GRADER_H
