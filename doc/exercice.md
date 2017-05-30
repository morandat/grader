Grader
======

- TOC
{:toc}

Overview
--------

Grader allows to create `C` exercises for students. It includes an unit test
framework,  provides an build system and allows to deploy exercises as
standalone or as part of an exam.

- From the student point of view, they can use (provided) functions even if they
don't write them. This allows to create complete and complex exercise.

- From the teacher point of view, grader allows quick prototyping of exercise.
They can provide some (default) code by using standard C syntax, and use a rich
Unit testing library.

Layout
------

An exercise should conform to a standard layout which can be extended.

`description.yml` or `description.md`
: Describe the exercise, i.e., name, tag, etc. (**Mandatory** for on-line
  exercise)

`Makefile.mk`
: Define features and dependency used by this exercice. It also defines
  forbidden functions. (**Mandatory**)

`exercice.c`
: Define test cases, oracles and provided functions. This file may (should)
  use the grader DSL. (**Mandatory**)

`enonce.c`
: The file that will be given to students (i.e., only defines, types and
  prototype). When using `provided.c`, this file will be derived from it.
  (**Mandatory** but may be derived from `provided.c`, see feature #provided)

`provided.c`
: Defines at same time the subject (the one given to student) and the provided
  functions. Its syntax has to follow a few convention (see feature #provided).

`prolog.c` and `epilog.c`
: Special files that may be prepended or appended to the answer of the student.
  In order to use them `Makefile.mk` should declare them as `OBJS` with their
  original filenames (i.e., `prolog.c` and not `prolog.o`). They also affect
  the provided feature even if they are not declared in `OBJS`.

Provided
--------

This allows to maintain the code for provided function in the same file than
the subject. To enable this feature `Makefile.mk` should contain
`OBJS=provided.o` (which adds the file in `libexam.a`) and `DEPS=provided.h
enonce.c` (ensures that we extract the right definitions and create the right
subject). Finally `exercice.c` should include `provided.h`.

`provided.c` can be any valid `C` file. However in order to extract properly
headers from it, two things have to be respected :

- Lines between `/^{/` and `/^}/` will be stripped out (this is called
  a function block).
- Provided function signature should be immediately followed by function block.
  They also have to start at the first column, may spawn on more than one line
  and should avoid macro.

Finally even when not specified by the `OBJS` directive, `prolog.c` and
`epilog.c` files alter the extraction process. Extraction of `provided.h` (not
`enonce.c`) will be performed on the concatenation of
`prolog.c provided.c epilog.c` in this order.

More files
----------

You can include any file you like in `libexam.a` solely by naming them in the
`OBJS` directive. Library can also be specified by `LDLIBS` however they should
be installed on the host system (which can be tricky in the online version).

Creating an exercise
--------------------

To quickly start an exercise from scratch:

`make EXO=name new`

This should provided the default layout with a simple example.

Simple test cases and function
------------------------------

The default layout allows you to create a test by providing:

- A function (either directly or by using the provided feature)
- A test case (should be declared before the test function)
- A test function
- Registering it in the test list (via EXERCICE or TEST_LIST)

~~~
#include "grader2.h"

PROVIDED_FUNCTION(int, foo, int bar)
{
  // Will provide "directly" `int foo(int bar)`.
  // See provided feature for alternatives
  return bar * 2;
}

TEST_CASES(foo, int expected, int provided)
{
  {2, 1},
  {4, 2},
  {-4, -2}, // Note: this last comma is optinal, but convenient 
}; // Note the semi column which is mandatory

TEST_FUNCTION(foo) // By default foo will be used with the `foo` test case
{
  FOR_EACH_TEST { // This provide a test case (instance of `foo` test case)
                  // under the name `_` which aliases `tests_values[test_idx]`
    int actual = DESCRIBE(foo(_.provided),
                        "foo(%d)", _.provided);
                  // DESCRIBE annotates any call, followed by a printf
                  // compatible format string. The call will be reported in
                  // case of a failing assertion. It is also usable via the
                  // global variable `last_call_buffer`
    int expected = __foo(_.provided); // To ensure calling a provided function,
                                      // the name has to be prepended by `__`
                                      // NB: expected is unused in this example
    ASSERT_INT(_.expected, actual, AS_RETURN); // Assert that `actual` is the
                  // same than `_.expected`. The last arguments refers to what
                  // is tested, here the return value.
  }
}

EXERCICE("example", foo); // Create the exercice `example` containing only the
                        // test `foo`
~~~

If the default layout does not meet your needs, you can customize it with some
additional work (see Advanced test functions).

Additionally a `NO_TEST_CASE(foo);` exist. However you will have to call either
`UNUSED_TEST_CASES;` or `FOR_EACH_TEST` to mute warnings and activate the test
fixture (see Fixture).

Extension to printf
-------------------

At any place where a `printf` compatible format string is expected, a few
exotics formats are available. Note that all this formats will expect two
parameters, one for the number of items (int) and a pointer to the printed
values.

Predefined extension:

`%D`, `%#D`, `%+D`
: Array of decimal. `#` will use hexadecimal. `+` uses unsigned.

`%S`, `%P`
: Array of quoted strings. Array of pointers.

`%@`, `%[`
: Array of structures. The former uses the main renderer, the later uses the
  alternative one. Renderer are explained in the following section.
  (TODO or not: Replace `%[` by `%#@`)
  (Not really working). The `+` modifier add new line between each values.

All these format use braces to surround list. To remove them, use the `-`
modifier, e.g., `"%-@", 1, some_ptr`


Renderer and comparators
------------------------

Graders allows you to quickly define custom `struct` renderer and comparator.

~~~
struct some_struct  {
  int x, y;
  char *label;
}

PRINT_RENDERER(some_struct, s,
  "(%d, %d): %s", s->x, s->y, s->label)

DEFINE_STRUCT_COMPARATOR(some_struct)

TEST_CASES(foo, struct some_struct expected, struct some_struct provided)
{
  {{1, 2, "tada"}, {2, 1, "tada"}},
};

TEST_FUNCTION(foo)
{
  struct_renderer(some_struct_renderer);
  FOR_EACH_TEST {
    struct some_struct actual = DESCRIBE(foo(_.provided),
                                        "foo(%-@)", 1, &_.provided);
    ASSERT_STRUCT(compare_some_struct, &_.expected, &actual, AS_RETURN);
  }
}
~~~

> Default comparator pitfall: default comparator uses memcpy. Thus they are
> very sensitive to memory alignement (padding) and very sensitive to arrays.
> In case of any doubt, a custom comparator should be provided.
> Comparators are normal `C` predicates takings two pointers. Nothing more is
> required (naming, storage).

~~~
static int compare_automaton(const struct automaton *a1,
                             const struct automaton *a2)
{
    if (a1->initial != a2->initial) return 0;
    if (a1->state_count != a2->state_count) return 0;
    for (int i = 0; i < a1->state_count; i ++)
      if (!compare_state(&a1->states[i], &a2->states[i]))
        return 0;
    return 1;
}
~~~

### Custom renderer

Unlike comparators, custom structure renderers are harder to get them right.

~~~
CUSTOM_PRINT_RENDERER(automaton, array_automaton); // define a custom renderer
static int array_automaton(FILE *stream, const char *fmt, const struct automaton* a)
{   // this signature is imposed, it should return the number of char printed
    // like any XXprintf function.
    int len = fprintf(stream, fmt); // This prints the surronding `{` if needed
    len += fprintf(stream, "{ %d, ", a->state_count);
    struct printf_info info = {.showsign = 1}; // Force line skip
    CALL_PRINT_RENDERER(len, render_state, a->state_count, &a->states);
    len += fprintf(stream, "}");
    return len;
}

// Note this feature is highly instable and may be improved (especially the
// info part).
~~~

Fixture
-------

There are three levels of fixtures available in grader, they should be used at
most once by `exercice.c`

`GLOBAL_SETUP`
: Will be triggered once before any test. This directive can appears anywhere
  in the `exercice.c`

`LOCAL_SETUP`
: Will be triggered once by test function. The local fixture is the only one
  allowing parameters (see example after). This directive should appear before
  all test functions.

`TEST_SETUP`
: Will be triggered before any test case, i.e., before each loop of
  `FOR_EACH_TEST`

~~~
GLOBAL_SETUP {
  some_safe_global_var = some_init_function();
}

LOCAL_SETUP(struct renderer renderer) {
  struct_renderer(renderer);
}

TEST_SETUP {
  some_global_var = some_safe_global_var;
}

...

TEST_FUNCTION(foo, some_struct_renderer)
{
...
}
~~~

Assertions
----------

Grader provides a lot of predefined assertions for built-in types and user
define ones. It also provides some very specific asserts for memory management,
recursive function call, assert, ...

> All these asserts are macros, thus consider avoiding using function call or
> side effects in their arguments

Generic asserts
: `ASSERT_EXACT_MSG(expected, actual, ...)` uses `...` in case where `==`
  returns zero. `ASSERT_FP_MSG(expected, actual, prec, ...)` does the same up
  to some precision, i.e., for floating points types.

All asserts uses under the hood: `PASSED()`, `FAILED(...)`. Obviously you can
create custom asserts by using directly this macros.

Exact types
: `ASSERT_XXX(expected, actual, TAG)`. `XXX` is one of `INT`, `UINT`, `LONG`,
  `ULONG`, `LONGLONG`, `ULONGLONG`, `CHAR`, `PTR`, `STRING`.

Floating point types
: `ASSERT_XXX(expected, actual, precission, TAG)`. `XXX` is one of `FLOAT` or
  `DOUBLE`

Special asserts
: `ASSERT_XXX(actual, TAG)`. `XXX` is one of `NULL`, `NOT_NULL`, `TRUE`, `FALSE`

Array of built-in asserts
: `ASSERT_ARRAY(size, expected, actual, fmt, tag)`. `size` is the size of both
  array (it's your job to check it before). Both arrays should be pointers.
  `fmt` is a `printf` specifier that may be used with each element of the array.
  There also exists an `ASSERT_ARRAY_UNORDERED`.

Struct and array of struct assert
: `ASSERT_STRUCT(comparator, expected, actual, tag)`. Assert that the two
  pointers (`expected` and `actual`) points to comparable elements.
  Only the primary struct renderer is used with this assertion.
  `NULL` is only comparable with itself. For arrays of structs you can use
  `ASSERT_STRUCT_ARRAY(comparator, size, expected, actual, tag)`, or 
  `ASSERT_STRUCT_ARRAY_UNORDERED`.

Is a function implemented
: `ASSERT_IMPLEMENTED(fun)`. `ASSERT_IMPLEMENTED_SILENT` silently fails if a
  function is not implemented.

Padding
: `ASSERT_STILL_PADDED(len, tab, tag)` ensure that a padded buffer is still
  correct (i.e., no buffer overflow). There is currently no such feature for
  buffer underflow. To pad an array, you can use `PAD_BUFFER(array)` or
  `PAD_DYNAMIC_BUFFER(array, element_count)`

The following assert may change at any time.

Trace function and recusion
: TODO `TRACE_CALL`, `ASSERT_RECURSIVE`, `ASSERT_NOT_RECURSIVE`

Check memory
: TODO `__BEGIN_MEMORY_CHECKING__`, `__END_MEMORY_CHECKING__`,
  `ASSERT_MEMORY_BALANCED`, `FREE_ALL`

Assert assertion and exits
: TODO `ASSERT_RAISES`, `ASSERT_NO_RAISES`, `ASSERT_EXIT_WITH`

### Tags

Most of high level asserts use a `tag`. The tag is small message that will be
added to any assertion failure. Most generics tags are provided, but any
literal string may be used (preferably followed by a space, e.g., "array to
have the same size ").

`AS_RETURN` (alias `AS_RESULT`)
: When comparing return value

`AS_PARAM(x)`
: When comparing a parameter named `x` for side effects

`AS_CONTAINED_BY(x)`
: When expecting a single item to belong to an array passed as parameter `x`

`AS_PRINTED`
: When expecting something to be printed

Calling provided function
-------------------------

Provided function are renamed under the hood by the framework, which actually
allows the student to define it's own. To ensure calling the provided version
and not the one overridded by the student you have to prepend the function name
by `__`.

~~~
PROVIDED_FUNCTION(int, foo, void) { }

...
__foo(); // calls the original
foo(); // calls either the student version or the original version
~~~

If you really wanna check yourself which is which you may rely on :

~~~
if (foo == __foo) {
  // That's the provided version
} else {
  // It's the student one
}
~~~

Describe call
-------------

It is preferable to describe the call to any student function, thus some
feedback can be given in case of assert fail or segfault.

~~~
  int actual = DESCRIBE(foo(_.provided),
                        "foo(%d)", _.provided);
~~~

Actually `DESCRIBE` sets via `snprintf` `last_call_buffer`. You may directly
write inside this buffer or use it in any print statement.

Asserting prints
----------------

In order to assert any print done you'll have to grab them first.

~~~
 start_grab_printf();
 DESCRIBE(hello_world(), "hello_world()");
 char *actual = end_grab_printf();
 ASSERT_STRING(_.expected, actual, AS_PRINTED);
~~~

Registering tests
-----------------

The easiest and quickest way to register test functions is to use:

~~~
EXERCICE("Some exercice", foo, bar, baz);
~~~

which is a shortcut to a slightly more verbose version:

~~~
BEGIN_TESTS_LIST
  TESTS("Some exercice", foo, bar, baz)
END_TESTS_LIST
~~~

Unfortunately those macros won't work with more custom exercises. For instance
if you have more than one test cases by function or if you try to reuse test
cases for another function. All these cases require to use the primitive form:

~~~
BEGIN_TESTS_LIST
  TEST("Some exercice:foo", foo, bar) // Uses the foo function with the bar
                                      // test cases.
  // If you want to override completely the framework, you have to use :
  { "Some exercice:weird fun", (testfun_t) your_function,
  number_of_test_cases, a_pointer_to_your_test_cases },
END_TESTS_LIST
~~~


Advanced test functions
-----------------------

Test functions are directly (i.e., by name) bound to both, a provided function
and a test case. Obviously this behaviour can be overridden.

Test function which depends on another provided function
: Simply replace `TEST_FUNCTION` by `TEST_FUNCTION_WITH(test_name, fun_name)` .
  As with the former version you can also pass arguments to local setup.

Defining test case with a same shape than another one
: `TEST_CASES_LIKE(foo, bar)` will define tests cases that will have the same
  shape than `bar` for the `foo` test function. If you register the test
  manually it may be useful to have more than one test cases for one function.

Reusing the same test cases for another function
: `SAME_TEST_CASES(foo, bar)` will reuse the `bar` test cases for `foo`.
  Unfortunately according to the way test cases are defined, you HAVE to
  register manually such test function.


Memory Management
-----------------

TODO
