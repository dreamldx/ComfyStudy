#include <iostream>

#include "greeting.h"
#include "net_probe.h"

int main() {
  std::cout << Greeting() << '\n';
  std::cout << CurlVersion() << '\n';
  return 0;
}
