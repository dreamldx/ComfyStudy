#include <string>

#include "check.h"
#include "greeting.h"
#include "version.h"

int main() {
  CHECK_EQ(Greeting(), std::string("ComfyStudy ") + COMFYSTUDY_VERSION);
  CHECK(!Greeting().empty());
  return g_failures == 0 ? 0 : 1;
}
