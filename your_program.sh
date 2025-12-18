#!/bin/sh
#
# Use this script to run your program LOCALLY.
#
# Note: Changing this script WILL NOT affect how CodeCrafters runs your program.
#
# Learn more: https://codecrafters.io/program-interface

set -e # Exit early if any commands fail

# Copied from .codecrafters/compile.sh
#
# - Edit this to change how your program compiles locally
# - Edit .codecrafters/compile.sh to change how your program compiles remotely
(
  cd "$(dirname "$0")" # Ensure compile steps are run within the repository directory

  # Choose a generator: prefer Ninja if available; otherwise let CMake default.
  GEN_ARGS=""
  if command -v ninja >/dev/null 2>&1; then
    GEN_ARGS="-G Ninja"
  fi

  # Use vcpkg toolchain if VCPKG_ROOT is set and valid; otherwise build without it.
  TOOLCHAIN_ARGS=""
  if [ -n "${VCPKG_ROOT:-}" ] && [ -f "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" ]; then
    TOOLCHAIN_ARGS="-DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
  else
    echo "Warning: VCPKG_ROOT is not set or invalid; building without vcpkg toolchain."
    echo "         Set VCPKG_ROOT to your vcpkg path to auto-resolve OpenSSL/Zlib."
  fi

  # Configure and build (Release by default for single-config generators).
  cmake -B build -S . ${GEN_ARGS} -DCMAKE_BUILD_TYPE=Release ${TOOLCHAIN_ARGS}
  cmake --build ./build --config Release
)

# Copied from .codecrafters/run.sh
#
# - Edit this to change how your program runs locally
# - Edit .codecrafters/run.sh to change how your program runs remotely
exec $(dirname "$0")/build/git "$@"
