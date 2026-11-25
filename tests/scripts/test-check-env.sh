#!/usr/bin/env bash
# Tests for scripts/check-env.sh without touching the real system: each case builds a directory of fake
# commands and runs the script with PATH set to that directory only, so the real compilers cannot leak in.
# Usage: bash tests/scripts/test-check-env.sh
# Exit 0 if every case passes, 1 otherwise.
set -u

here="${BASH_SOURCE[0]%/*}"
script="$here/../../scripts/check-env.sh"
bash_bin="$(command -v bash)"

passed=0
failed=0
tmp_root="$(mktemp -d)"
trap 'rm -rf "$tmp_root"' EXIT

# make_fake DIR NAME BODY: an executable called NAME in DIR whose body is a /bin/sh script.
make_fake() {
  printf '#!/bin/sh\n%s\n' "$3" > "$1/$2"
  chmod +x "$1/$2"
}

# make_gcc DIR NAME VERSION MACHINE: a fake compiler that answers -dumpfullversion and -dumpmachine.
make_gcc() {
  make_fake "$1" "$2" "case \"\$1\" in -dumpfullversion) echo $3;; -dumpmachine) echo $4;; esac"
}

# new_good_dir: a directory in which every tool the script wants is present and acceptable.
new_good_dir() {
  local dir
  dir="$(mktemp -d "$tmp_root/case.XXXXXX")"
  make_gcc "$dir" g++-16 16.2.0 x86_64-linux-gnu
  make_gcc "$dir" gcc-16 16.2.0 x86_64-linux-gnu
  # The unversioned aliases must be ignored, so give them a different major version.
  make_gcc "$dir" g++ 99.0.0 x86_64-linux-gnu
  make_gcc "$dir" gcc 99.0.0 x86_64-linux-gnu
  make_fake "$dir" cmake 'echo "cmake version 4.4.5"; echo; echo "CMake suite maintained and supported by Kitware"'
  make_fake "$dir" ninja 'echo 1.13.2'
  make_fake "$dir" pkg-config 'echo 2.5.1'
  make_fake "$dir" git 'echo "git version 2.55.0"'
  make_fake "$dir" ld 'echo "GNU ld (GNU Binutils for Debian) 2.46"'
  echo "$dir"
}

# run_check DIR [ARGS...]: runs the script with only DIR on PATH; sets $out and $rc.
run_check() {
  local dir="$1"
  shift
  out="$(PATH="$dir" "$bash_bin" "$script" "$@" 2>&1)"
  rc=$?
}

# expect NAME WANT_RC PATTERN...: checks the exit code and that every PATTERN is a line (or part of a line) in $out.
expect() {
  local name="$1" want_rc="$2" ok=1
  shift 2
  if [ "$rc" -ne "$want_rc" ]; then
    ok=0
    echo "  exit code $rc, wanted $want_rc"
  fi
  local pattern
  for pattern in "$@"; do
    if [[ "$out" != *"$pattern"* ]]; then
      ok=0
      echo "  output lacks: $pattern"
    fi
  done
  if [ "$ok" -eq 1 ]; then
    passed=$((passed + 1))
    echo "PASS $name"
  else
    failed=$((failed + 1))
    echo "FAIL $name"
    echo "$out" | sed 's/^/    | /'
  fi
}

# expect_absent NAME PATTERN: $out must not contain PATTERN.
expect_absent() {
  if [[ "$out" == *"$2"* ]]; then
    failed=$((failed + 1))
    echo "FAIL $1 (output contains: $2)"
  else
    passed=$((passed + 1))
    echo "PASS $1"
  fi
}

dir="$(new_good_dir)"
run_check "$dir" --no-smoke
expect "everything present" 0 "OK      g++-16: 16.2.0" "OK      gcc-16: 16.2.0" "OK      cmake: cmake version 4.4.5" "OK      ninja: version 1.13.2" "check-env: OK"
expect_absent "an install hint is not printed when all is well" "install:"
expect_absent "the unversioned g++ alias is not looked at" "99.0.0"

dir="$(new_good_dir)"
rm "$dir/g++-16"
run_check "$dir" --no-smoke
expect "g++-16 missing" 1 "MISSING g++-16" "check-env: FAILED, missing: g++-16" "install: sudo apt install g++-16"

dir="$(new_good_dir)"
make_gcc "$dir" g++-16 15.2.0 x86_64-linux-gnu
run_check "$dir" --no-smoke
expect "g++-16 reports another major version" 1 "MISSING g++-16: major version 15, need 16"

dir="$(new_good_dir)"
make_gcc "$dir" g++-16 16.2.0 aarch64-linux-gnu
run_check "$dir" --no-smoke
expect "compiler target is not x86_64" 1 "MISSING g++-16: target is not x86_64 (aarch64-linux-gnu)"

dir="$(new_good_dir)"
make_fake "$dir" cmake 'echo "cmake version 3.24.4"'
run_check "$dir" --no-smoke
expect "cmake too old" 1 "MISSING cmake: CMake 3.25 or newer is required" "install: sudo apt install cmake"

dir="$(new_good_dir)"
make_fake "$dir" cmake 'echo "something unexpected"'
run_check "$dir" --no-smoke
expect "cmake version unreadable" 1 "MISSING cmake: cannot read the CMake version"

dir="$(new_good_dir)"
rm "$dir/ninja" "$dir/gcc-16"
run_check "$dir" --no-smoke
expect "several tools missing, packages listed once each" 1 "check-env: FAILED, missing: gcc-16, ninja" "install: sudo apt install gcc-16 ninja-build"

dir="$(new_good_dir)"
rm "$dir/ld"
run_check "$dir" --no-smoke
expect "ld missing maps to binutils" 1 "MISSING ld" "install: sudo apt install binutils"

dir="$(new_good_dir)"
run_check "$dir" --frobnicate
expect "unknown option" 2 "usage:"

echo
echo "check-env tests: $passed passed, $failed failed"
[ "$failed" -eq 0 ]
