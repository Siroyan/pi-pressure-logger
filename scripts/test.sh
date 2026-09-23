#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p .test-build
"${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror -Wno-sign-compare -Wno-unused-parameter \
  -g -fsanitize=address,undefined -fno-omit-frame-pointer \
  -Itest/fakes -Isrc -Itest/host \
  test/host/*.cpp src/SDManager.cpp src/TimeManager.cpp src/WiFiManager.cpp src/DisplayManager.cpp \
  -o .test-build/host-tests
.test-build/host-tests
