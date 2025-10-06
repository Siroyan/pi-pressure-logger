CXX = g++
CXXFLAGS = -std=c++11 -DUNIT_TEST -I. -Wall -Wextra -g
TARGET_DIR = build
TARGETS = test_simple test_state_manager test_time_manager test_sd_manager_logic test_data_processing

# Unity testing framework (download if not present)
UNITY_URL = https://raw.githubusercontent.com/ThrowTheSwitch/Unity/master/src
UNITY_FILES = unity.c unity.h unity_internals.h

all: $(TARGETS)

# Download Unity framework if not present
unity.h:
	@echo "Downloading Unity testing framework..."
	@curl -s -O $(UNITY_URL)/unity.c
	@curl -s -O $(UNITY_URL)/unity.h
	@curl -s -O $(UNITY_URL)/unity_internals.h

# Simple test without Unity
test_simple: test_simple.cpp test_helpers.h
	$(CXX) $(CXXFLAGS) test_simple.cpp -o test_simple

# Unity-based tests
test_state_manager: test_state_manager.cpp test_helpers.h unity.h
	$(CXX) $(CXXFLAGS) test_state_manager.cpp unity.c -o test_state_manager

test_time_manager: test_time_manager.cpp test_helpers.h unity.h
	$(CXX) $(CXXFLAGS) test_time_manager.cpp unity.c -o test_time_manager

test_sd_manager_logic: test_sd_manager_logic.cpp test_helpers.h unity.h
	$(CXX) $(CXXFLAGS) test_sd_manager_logic.cpp unity.c -o test_sd_manager_logic

test_data_processing: test_data_processing.cpp test_helpers.h unity.h
	$(CXX) $(CXXFLAGS) test_data_processing.cpp unity.c -o test_data_processing

# Run all tests
test: all
	@echo "Running all tests..."
	@echo "=== Simple Test ==="
	@./test_simple || echo "test_simple failed"
	@echo "=== State Manager Test ==="
	@./test_state_manager || echo "test_state_manager failed"
	@echo "=== Time Manager Test ==="
	@./test_time_manager || echo "test_time_manager failed"
	@echo "=== SD Manager Logic Test ==="
	@./test_sd_manager_logic || echo "test_sd_manager_logic failed"
	@echo "=== Data Processing Test ==="
	@./test_data_processing || echo "test_data_processing failed"

# Clean built files
clean:
	rm -f $(TARGETS) unity.c unity.h unity_internals.h

.PHONY: all test clean