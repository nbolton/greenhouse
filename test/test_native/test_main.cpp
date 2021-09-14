#include "test_system.h"
#include "test_heating.h"

#include <iostream>

#include <unity.h>

void process()
{
  UNITY_BEGIN();
  testSystem();
  testHeating();
  UNITY_END();
}

int main(int argc, char **argv)
{
  process();
  return 0;
}
