#ifdef ARDUINO

#include "embedded/System.h"

using namespace greenhouse::embedded;

System s;

void setup()
{
  System::Instance(s);
  s.Setup();
}

void loop() { s.Loop(); }

#else

#include "native/System.h"

#if 0

using namespace greenhouse::embedded;

System s;

int main()
{
  s.Setup();
  while(true) {
    s.Loop();
  }
}
#endif // 0

#endif
