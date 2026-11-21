#pragma once

#include <iostream>

// A tiny test harness: CHECK/CHECK_EQ count failures instead of stopping, and each test
// program returns a non-zero exit code when any check failed (that is what CTest looks at).
inline int g_failures = 0;

#define CHECK(expr)                                                              \
  do {                                                                           \
    if (!(expr)) {                                                               \
      ++g_failures;                                                              \
      std::cerr << __FILE__ << ':' << __LINE__ << ": CHECK failed: " #expr "\n"; \
    }                                                                            \
  } while (false)

#define CHECK_EQ(a, b)                                                         \
  do {                                                                         \
    const auto& check_left = (a);                                              \
    const auto& check_right = (b);                                             \
    if (!(check_left == check_right)) {                                        \
      ++g_failures;                                                            \
      std::cerr << __FILE__ << ':' << __LINE__ << ": CHECK_EQ failed: " #a     \
                << " == " #b "\n  left:  " << check_left                      \
                << "\n  right: " << check_right << '\n';                      \
    }                                                                          \
  } while (false)
