#include <TokenIterator.h>
#include <unity.h>
#include <string.h>

char buffer[100];

void test_empty(void) {
  strcpy(buffer, "");

  TokenIterator itr(buffer, 0);

  TEST_ASSERT_EQUAL(false, itr.hasNext());
}

void test_simple(void) {
  const char* testcase = "a/b/c";

  strcpy(buffer, testcase);

  TokenIterator itr(buffer, strlen(testcase), '/');

  TEST_ASSERT_EQUAL(true, itr.hasNext());
  TEST_ASSERT_EQUAL_STRING("a", itr.nextToken());

  TEST_ASSERT_EQUAL(true, itr.hasNext());
  TEST_ASSERT_EQUAL_STRING("b", itr.nextToken());

  TEST_ASSERT_EQUAL(true, itr.hasNext());
  TEST_ASSERT_EQUAL_STRING("c", itr.nextToken());

  TEST_ASSERT_EQUAL(false, itr.hasNext());
}

void test_blank_tokens(void) {
  const char* testcase = "a//b/c";

  strcpy(buffer, testcase);

  TokenIterator itr(buffer, strlen(testcase), '/');

  TEST_ASSERT_EQUAL(true, itr.hasNext());
  TEST_ASSERT_EQUAL_STRING("a", itr.nextToken());

  TEST_ASSERT_EQUAL(true, itr.hasNext());
  TEST_ASSERT_EQUAL_STRING("", itr.nextToken());

  TEST_ASSERT_EQUAL(true, itr.hasNext());
  TEST_ASSERT_EQUAL_STRING("b", itr.nextToken());

  TEST_ASSERT_EQUAL(true, itr.hasNext());
  TEST_ASSERT_EQUAL_STRING("c", itr.nextToken());

  TEST_ASSERT_EQUAL(false, itr.hasNext());
}

void process() {
    UNITY_BEGIN();
    RUN_TEST(test_empty);
    RUN_TEST(test_simple);
    RUN_TEST(test_blank_tokens);
    UNITY_END();
}

int main(int argc, char **argv) {
    process();
    return 0;
}