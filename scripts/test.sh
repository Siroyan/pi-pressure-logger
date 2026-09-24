#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p .test-build
for mode in online offline; do
  mode_flags=(-DPRESSURE_TEST_CONFIG)
  if [[ "$mode" == offline ]]; then mode_flags+=(-DPRESSURE_OFFLINE); fi
"${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror -Wno-sign-compare -Wno-unused-parameter \
  "${mode_flags[@]}" -g -fsanitize=address,undefined -fno-omit-frame-pointer \
  -Itest/fakes -Isrc -Itest/host \
  test/host/*.cpp src/SDManager.cpp src/TimeManager.cpp src/WiFiManager.cpp src/DisplayManager.cpp src/StateManager.cpp src/MQTTManager.cpp src/NetworkService.cpp src/LoggerApplication.cpp \
  -o ".test-build/host-tests-$mode"
".test-build/host-tests-$mode"
done
