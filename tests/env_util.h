#pragma once

#include <stdlib.h>

// Sets an environment variable for the test program. An empty value removes the variable on every system
// (on Windows that is what _putenv_s does with ""), so a test can start from "not set".
inline void SetEnv(const char* name, const char* value) {
#if defined(_WIN32)
  _putenv_s(name, value);
#else
  if (*value == '\0') {
    unsetenv(name);
  } else {
    setenv(name, value, 1);
  }
#endif
}
