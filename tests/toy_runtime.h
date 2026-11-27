#pragma once

#include <memory>

#include "native_tools.h"
#include "tool_runtime.h"

// A tool runtime with the toy tools of chapters 1.3 and 1.4 (add, sleep, repeat, shutdown) in it.
inline agent::ToolRuntime ToyRuntime(agent::RuntimeOptions options = {}) {
  agent::ToolRuntime runtime(options);
  auto native = std::make_shared<agent::NativeTools>();
  agent::AddToyTools(*native);
  runtime.AddSource(native);
  return runtime;
}
