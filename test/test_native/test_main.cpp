#include "test_common.h"

#include <unity.h>

void process()
{
  UNITY_BEGIN();
  testCommon();
  UNITY_END();
}

int main(int argc, char **argv)
{
  process();
  return 0;
}