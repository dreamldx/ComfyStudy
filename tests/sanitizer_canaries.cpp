// Canaries for the sanitizers: each argument triggers one real bug, and the tests in
// cmake/platform/linux.cmake pass only if the right sanitizer reports it. If a sanitizer is not switched on
// (or is switched off by an environment variable), its canary fails, so a green sanitizer run means something.
//
// The bad values go through volatile variables and noinline functions on purpose. The project builds with
// -Wall -Wextra -Werror, and at -O2 GCC finds a plain a[4] on an int[4] at compile time and stops the build;
// this way the file compiles at any optimisation level and only the sanitizers can see the bugs.
#include <climits>
#include <cstddef>
#include <iostream>
#include <string>

namespace {

volatile int g_four = 4;
volatile int g_zero = 0;
volatile int g_forty = 40;
int g_global[4] = {1, 2, 3, 4};
int* volatile g_sink = nullptr;

__attribute__((noinline)) int At(const int* p, int i) { return p[i]; }
__attribute__((noinline)) void Release(int* p) { delete[] p; }
__attribute__((noinline)) void ReleaseOne(int* p) { delete p; }
__attribute__((noinline)) int* DanglingLocal() {
  int local[4] = {1, 2, 3, 4};
  int* volatile escape = local;
  return escape;
}

int HeapOverflow() {
  int* p = new int[4]{1, 2, 3, 4};
  const int v = At(p, g_four);
  Release(p);
  return v;
}

int HeapUseAfterFree() {
  int* p = new int[4]{1, 2, 3, 4};
  Release(p);
  return At(p, 0);
}

int StackOverflow() {
  int a[4] = {1, 2, 3, 4};
  return At(a, g_four);
}

int StackUseAfterReturn() { return At(DanglingLocal(), 0); }

int GlobalOverflow() { return At(g_global, g_four); }

int Leak() {
  g_sink = new int[16];
  g_sink = nullptr;  // the only pointer to the block is gone
  return 0;
}

int DoubleFree() {
  int* p = new int(1);
  ReleaseOne(p);
  ReleaseOne(p);
  return 0;
}

int SignedOverflow() {
  int x = INT_MAX;
  x += g_four;
  return x;
}

int ShiftTooFar() { return 1 << g_forty; }

int DivideByZero() { return 10 / g_zero; }

int NullDeref() {
  int* volatile p = nullptr;
  return *p;
}

}  // namespace

int main(int argc, char** argv) {
  const std::string what = argc > 1 ? argv[1] : "";
  if (what == "heap-overflow") return HeapOverflow();
  if (what == "heap-use-after-free") return HeapUseAfterFree();
  if (what == "stack-overflow") return StackOverflow();
  if (what == "stack-use-after-return") return StackUseAfterReturn();
  if (what == "global-overflow") return GlobalOverflow();
  if (what == "leak") return Leak();
  if (what == "double-free") return DoubleFree();
  if (what == "signed-overflow") return SignedOverflow();
  if (what == "shift") return ShiftTooFar();
  if (what == "div-zero") return DivideByZero();
  if (what == "null") return NullDeref();
  std::cout << "ok\n";
  return 0;
}
