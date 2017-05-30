#include "grader2.h"
#include "provided.h"

TEST_CASES(copy_upcased, const char *value, const char *expected)
{
  { "hello", "HELLO" },
};

TEST_FUNCTION(copy_upcased)
{
  FOR_EACH_TEST {
    char actual[10];
    PAD(actual);
    DESCRIBE(copy_upcased(actual, _.value),
        "copy_upcased(dst, \"%s\")", _.value);

    ASSERT_STRING(_.expected, actual, AS_PARAM(dst));
    ASSERT_STILL_PADDED(strlen(_.expected) + 1, actual, AS_PARAM(dst));
  }
}

EXERCICE("exemple", copy_upcased);
