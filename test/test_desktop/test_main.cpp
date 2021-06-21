#include <unity.h>

// void setUp(void) {
// // set stuff up here
// }

// void tearDown(void) {
// // clean stuff up here
// }

void test_hello_world(void) { TEST_ASSERT_EQUAL(42, 42); }

void process()
{
  UNITY_BEGIN();
  RUN_TEST(test_hello_world);
  UNITY_END();
}

int main(int argc, char **argv)
{
  process();
  return 0;
}
