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
  make_fake "$dir" curl 'echo "curl 8.24.0"'
  make_fake "$dir" zip 'echo "Zip 3.0"'
  make_fake "$dir" unzip 'echo "UnZip 6.00"'
  make_fake "$dir" tar 'echo "tar (GNU tar) 1.35"'
  make_fake "$dir" make 'echo "GNU Make 4.4.1"'
  make_fake "$dir" perl 'exit 0'
  # vcpkg is not on PATH: it lives in $VCPKG_ROOT, which run_check points at this directory by default.
  mkdir "$dir/vcpkg-root"
  make_fake "$dir/vcpkg-root" vcpkg 'echo "vcpkg package management program version 2026-11-09-fake"'
  echo "$dir"
}

# run_check DIR [ARGS...]: runs the script with only DIR on PATH; sets $out and $rc.
# VCPKG_ROOT is DIR/vcpkg-root unless the case sets vcpkg_root_case (an empty value means "not set").
run_check() {
  local dir="$1"
  shift
  out="$(PATH="$dir" VCPKG_ROOT="${vcpkg_root_case-$dir/vcpkg-root}" "$bash_bin" "$script" "$@" 2>&1)"
  rc=$?
  unset vcpkg_root_case
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
expect "everything present" 0 "OK      g++-16: 16.2.0" "OK      gcc-16: 16.2.0" "OK      cmake: cmake version 4.4.5" "OK      ninja: version 1.13.2" "OK      curl: curl 8.24.0" "OK      tar: tar (GNU tar) 1.35" "OK      vcpkg: vcpkg package management program version" "check-env: OK"
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
rm "$dir/curl"
run_check "$dir" --no-smoke
expect "curl missing" 1 "MISSING curl" "install: sudo apt install curl"

dir="$(new_good_dir)"
rm "$dir/zip" "$dir/unzip"
run_check "$dir" --no-smoke
expect "zip and unzip missing" 1 "check-env: FAILED, missing: zip, unzip" "install: sudo apt install zip unzip"

dir="$(new_good_dir)"
rm "$dir/make"
run_check "$dir" --no-smoke
expect "make missing (the openssl port needs it)" 1 "MISSING make" "install: sudo apt install make"

dir="$(new_good_dir)"
rm "$dir/perl"
run_check "$dir" --no-smoke
expect "perl missing" 1 "MISSING perl" "install: sudo apt install perl"

dir="$(new_good_dir)"
make_fake "$dir" perl 'exit 2'
run_check "$dir" --no-smoke
expect "perl cannot load IPC::Cmd" 1 "MISSING perl: the Perl module IPC::Cmd cannot be loaded (the openssl port needs it)"

# The baseline the hint tells you to check vcpkg out at is the one in the repository's vcpkg.json.
manifest="$(<"$here/../../vcpkg.json")"
baseline=""
[[ "$manifest" =~ \"builtin-baseline\"[[:space:]]*:[[:space:]]*\"([0-9a-f]{40})\" ]] && baseline="${BASH_REMATCH[1]}"

dir="$(new_good_dir)"
vcpkg_root_case=""
run_check "$dir" --no-smoke
expect "VCPKG_ROOT not set" 1 "MISSING vcpkg: VCPKG_ROOT is not set" "check-env: FAILED, missing: vcpkg" \
  "install: git clone https://github.com/microsoft/vcpkg ~/vcpkg && git -C ~/vcpkg checkout $baseline && ~/vcpkg/bootstrap-vcpkg.sh -disableMetrics"
expect_absent "no apt command is suggested when only vcpkg is missing" "sudo apt install"

dir="$(new_good_dir)"
mkdir "$dir/empty"
vcpkg_root_case="$dir/empty"
run_check "$dir" --no-smoke
expect "VCPKG_ROOT has no vcpkg in it" 1 "MISSING vcpkg: no executable at $dir/empty/vcpkg"

dir="$(new_good_dir)"
make_fake "$dir/vcpkg-root" vcpkg 'exit 1'
run_check "$dir" --no-smoke
expect "vcpkg does not run" 1 "MISSING vcpkg: cannot run"

# The compiler is pinned in two places; they must name the same major version.
script_text="$(<"$script")"
toolchain_text="$(<"$here/../../cmake/toolchains/linux-gcc.cmake")"
script_major="" toolchain_major=""
[[ "$script_text" =~ GCC_MAJOR=([0-9]+) ]] && script_major="${BASH_REMATCH[1]}"
[[ "$toolchain_text" =~ COMFYSTUDY_GCC_MAJOR[[:space:]]+([0-9]+) ]] && toolchain_major="${BASH_REMATCH[1]}"
if [ -n "$script_major" ] && [ "$script_major" = "$toolchain_major" ]; then
  passed=$((passed + 1))
  echo "PASS check-env.sh and the toolchain file pin the same GCC major version ($script_major)"
else
  failed=$((failed + 1))
  echo "FAIL check-env.sh pins '$script_major' but cmake/toolchains/linux-gcc.cmake pins '$toolchain_major'"
fi

dir="$(new_good_dir)"
run_check "$dir" --frobnicate
expect "unknown option" 2 "usage:"

echo
echo "check-env tests: $passed passed, $failed failed"
[ "$failed" -eq 0 ]
