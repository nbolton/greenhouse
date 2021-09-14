#ifdef ARDUINO

#include "../lib/embedded/greenhouse/System.h"

embedded::greenhouse::System greenhouse;

void setup()
{
  embedded::greenhouse::System::Instance(greenhouse);
  greenhouse.Setup();
}

void loop()
{
  greenhouse.Loop();
}

#else

#include "../lib/native/greenhouse/System.h"

native::greenhouse::System greenhouse;

int main()
{
  greenhouse.Setup();
  greenhouse.Loop();
}

#endif
