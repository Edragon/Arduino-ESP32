#include <UrlTokenBindings.h>
#include <unity.h>
#include <string.h>

char buffer1[100];
char buffer2[100];

void test_simple(void) {
  strcpy(buffer1, "a/:b/:c");
  strcpy(buffer2, "a/123/456");

  TokenIterator itr1(buffer1, strlen(buffer1), '/');
  TokenIterator itr2(buffer2, strlen(buffer2), '/');

  UrlTokenBindings bindings(itr1, itr2);

  TEST_ASSERT_EQUAL(false, bindings.hasBinding("a"));
  TEST_ASSERT_EQUAL(true, bindings.hasBinding("b"));
  TEST_ASSERT_EQUAL(true, bindings.hasBinding("c"));

  TEST_ASSERT_EQUAL_STRING("123", bindings.get("b"));
  TEST_ASSERT_EQUAL_STRING("456", bindings.get("c"));
}

void process() {
    UNITY_BEGIN();
    RUN_TEST(test_simple);
    UNITY_END();
}

int main(int argc, char **argv) {
    process();
    return 0;
}