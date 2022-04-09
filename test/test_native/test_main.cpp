#include "test.h"

#include <iostream>

#include <unity.h>

void process()
{
  UNITY_BEGIN();
  //testSystem();
  //testHeating();
  testTime();
  UNITY_END();
}

int main(int argc, char **argv)
{
  process();
  return 0;
}
