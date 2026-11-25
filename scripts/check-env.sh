#!/usr/bin/env bash
# Checks that the tools the project needs are present and usable on Linux.
# Usage: scripts/check-env.sh [--no-smoke]
#   --no-smoke  only look at the tools; skip the two small test compilations
# Exit 0 only if every check passes; otherwise lists what is missing, suggests an apt command, and exits 1.
# It only checks and prints a suggestion. It never installs anything and never changes the system.
set -u

# The compiler is pinned by its major version and always called by its versioned name (g++-N), never
# through the g++ alias, so a distribution that moves its default compiler does not change what we use.
# nvcc (chapter platform-1.5) must accept this major version as its host compiler.
GCC_MAJOR=16
CMAKE_MIN_MAJOR=3
CMAKE_MIN_MINOR=25

smoke=1
for argument in "$@"; do
  case "$argument" in
    --no-smoke) smoke=0 ;;
    *)
      echo "usage: scripts/check-env.sh [--no-smoke]"
      exit 2
      ;;
  esac
done

missing=()
packages=()

# add_package NAME: remember an apt package once.
add_package() {
  local existing
  for existing in "${packages[@]+"${packages[@]}"}"; do
    [ "$existing" = "$1" ] && return
  done
  packages+=("$1")
}

# first_line TEXT: the first line of TEXT, without needing head.
first_line() {
  printf '%s\n' "${1%%$'\n'*}"
}

# check_tool NAME PACKAGE DESCRIBE: DESCRIBE prints one line of detail for the tool found at the given path;
# a line starting with "FAIL:" means the tool is there but not acceptable.
check_tool() {
  local name="$1" package="$2" describe="$3" path text
  path="$(command -v "$name" 2>/dev/null)"
  if [ -z "$path" ]; then
    printf 'MISSING %s\n' "$name"
    missing+=("$name")
    add_package "$package"
    return
  fi
  text="$("$describe" "$name" "$path")"
  if [[ "$text" == FAIL:* ]]; then
    printf 'MISSING %s: %s\n' "$name" "${text#FAIL: }"
    missing+=("$name")
    add_package "$package"
    return
  fi
  printf 'OK      %s: %s\n' "$name" "$text"
}

describe_compiler() {
  local full machine major
  full="$("$1" -dumpfullversion 2>/dev/null)"
  major="${full%%.*}"
  if [[ ! "$major" =~ ^[0-9]+$ ]]; then
    echo "FAIL: cannot read the compiler version"
    return
  fi
  if [ "$major" -ne "$GCC_MAJOR" ]; then
    echo "FAIL: major version $major, need $GCC_MAJOR"
    return
  fi
  machine="$("$1" -dumpmachine 2>/dev/null)"
  if [[ "$machine" != x86_64-* ]]; then
    echo "FAIL: target is not x86_64 ($machine)"
    return
  fi
  echo "$full"
}

describe_cmake() {
  local line
  line="$(first_line "$("$1" --version 2>&1)")"
  if [[ ! "$line" =~ cmake\ version\ ([0-9]+)\.([0-9]+) ]]; then
    echo "FAIL: cannot read the CMake version ($line)"
    return
  fi
  local major="${BASH_REMATCH[1]}" minor="${BASH_REMATCH[2]}"
  if [ "$major" -lt "$CMAKE_MIN_MAJOR" ] || { [ "$major" -eq "$CMAKE_MIN_MAJOR" ] && [ "$minor" -lt "$CMAKE_MIN_MINOR" ]; }; then
    echo "FAIL: CMake $CMAKE_MIN_MAJOR.$CMAKE_MIN_MINOR or newer is required ($line)"
    return
  fi
  echo "$line"
}

describe_ninja() { echo "version $(first_line "$("$1" --version 2>&1)")"; }
describe_pkg_config() { echo "version $(first_line "$("$1" --version 2>&1)")"; }
describe_git() { first_line "$("$1" --version 2>&1)"; }
describe_ld() { first_line "$("$1" --version 2>&1)"; }

check_tool "g++-$GCC_MAJOR" "g++-$GCC_MAJOR" describe_compiler
check_tool "gcc-$GCC_MAJOR" "gcc-$GCC_MAJOR" describe_compiler
check_tool cmake cmake describe_cmake
check_tool ninja ninja-build describe_ninja
check_tool pkg-config pkg-config describe_pkg_config
check_tool git git describe_git
check_tool ld binutils describe_ld

# Not a check: the glibc version bounds which prebuilt libraries (libtorch, ONNX Runtime) can be used later.
if command -v ldd > /dev/null 2>&1; then
  printf 'INFO    glibc: %s\n' "$(first_line "$(ldd --version 2>&1)")"
fi

# A compiler that exists may still not work. Compile and run a small C++20 program (concepts, std::span, a
# coroutine), then the same program with AddressSanitizer and UndefinedBehaviorSanitizer.
run_smoke() {
  local cxx="g++-$GCC_MAJOR" dir output
  dir="$(mktemp -d)" || { echo "MISSING smoke: cannot create a temporary directory"; missing+=(smoke); return; }
  # shellcheck disable=SC2064
  trap "rm -rf '$dir'" RETURN
  cat > "$dir/smoke.cpp" << 'EOF'
#include <concepts>
#include <coroutine>
#include <cstdio>
#include <span>
#include <vector>

template <std::integral T>
T twice(T value) { return value * 2; }

struct Task {
  struct promise_type {
    int value = 0;
    Task get_return_object() { return Task{std::coroutine_handle<promise_type>::from_promise(*this)}; }
    std::suspend_never initial_suspend() noexcept { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    void return_value(int v) { value = v; }
    void unhandled_exception() {}
  };

  explicit Task(std::coroutine_handle<promise_type> h) : handle(h) {}
  Task(Task&& other) noexcept : handle(other.handle) { other.handle = nullptr; }
  ~Task() { if (handle) handle.destroy(); }
  int result() const { return handle.promise().value; }

  std::coroutine_handle<promise_type> handle;
};

Task compute(int x) { co_return twice(x) + 1; }

int sum(std::span<const int> values) {
  int total = 0;
  for (int v : values) total += v;
  return total;
}

int main() {
  const std::vector<int> values{1, 2, 3, 4};
  Task task = compute(20);
  const int total = sum(values) + task.result();
  if (total != 51) {
    std::printf("unexpected result %d\n", total);
    return 1;
  }
  return 0;
}
EOF
  if ! output="$("$cxx" -std=c++20 -Wall -Wextra -Werror "$dir/smoke.cpp" -o "$dir/smoke" 2>&1)"; then
    printf 'MISSING smoke: %s cannot compile a C++20 program with concepts, std::span and coroutines\n' "$cxx"
    printf '%s\n' "$output" | head -n 5 | sed 's/^/        /'
    missing+=(smoke)
    return
  fi
  if ! output="$("$dir/smoke" 2>&1)"; then
    printf 'MISSING smoke: the compiled C++20 program gave a wrong result (%s)\n' "$output"
    missing+=(smoke)
    return
  fi
  printf 'OK      smoke: %s compiled and ran a C++20 program (concepts, std::span, coroutine)\n' "$cxx"

  if ! output="$("$cxx" -std=c++20 -fsanitize=address,undefined -g "$dir/smoke.cpp" -o "$dir/smoke_san" 2>&1)"; then
    printf 'MISSING smoke: %s cannot link AddressSanitizer and UndefinedBehaviorSanitizer\n' "$cxx"
    printf '%s\n' "$output" | head -n 5 | sed 's/^/        /'
    missing+=(smoke)
    return
  fi
  if ! output="$("$dir/smoke_san" 2>&1)"; then
    printf 'MISSING smoke: the sanitizer build of the program failed to run\n'
    printf '%s\n' "$output" | head -n 5 | sed 's/^/        /'
    missing+=(smoke)
    return
  fi
  printf 'OK      smoke: AddressSanitizer and UndefinedBehaviorSanitizer linked and ran\n'
}

# The smoke test needs the compiler; if it is already reported missing there is nothing more to learn.
if [ "$smoke" -eq 1 ] && command -v "g++-$GCC_MAJOR" > /dev/null 2>&1; then
  case " ${missing[*]+"${missing[*]}"} " in
    *" g++-$GCC_MAJOR "*) ;;
    *) run_smoke ;;
  esac
fi

if [ "${#missing[@]}" -gt 0 ]; then
  names="${missing[*]}"
  echo "check-env: FAILED, missing: ${names// /, }"
  if [ "${#packages[@]}" -gt 0 ]; then
    echo "install: sudo apt install ${packages[*]}"
  fi
  exit 1
fi
echo "check-env: OK"
exit 0
