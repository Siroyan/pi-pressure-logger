# Unit Tests for Pressure Logger

This directory contains unit tests for the core business logic components of the pressure logger system.

## Test Structure

### Test Files
- `test_state_manager.cpp` - Tests for state machine logic and transitions
- `test_time_manager.cpp` - Tests for time formatting and NTP-related string processing
- `test_sd_manager_logic.cpp` - Tests for file management and parsing logic
- `test_data_processing.cpp` - Tests for sensor data conversion and processing

## Running Tests

### Prerequisites
- C++ compiler (g++ or clang++) with C++11 support
- Make build system
- Unity testing framework (automatically downloaded)

### Run All Tests
```bash
make test
```

### Run Specific Test
```bash
make test_state_manager && ./test_state_manager
make test_time_manager && ./test_time_manager
make test_sd_manager_logic && ./test_sd_manager_logic
make test_data_processing && ./test_data_processing
```

### PlatformIO Alternative
If PlatformIO is available:
```bash
pio test -e native
```

### Clean Build Files
```bash
make clean
```

## Test Configuration

Tests are configured to run in the `native` environment which:
- Uses the Unity testing framework
- Compiles for native platform (not Arduino hardware)
- Excludes hardware-dependent libraries
- Defines `UNIT_TEST` and `NATIVE_TEST` flags

## Test Results Summary

**42 tests, 0 failures** ✅

### StateManager Tests (11 tests)
- State transition correctness (STANDBY ↔ RECORDING ↔ FILE_LIST)
- Button handling logic for all states
- Edge cases and invalid transitions
- State-specific behavior validation

### TimeManager Tests (9 tests)
- NTP timestamp formatting (YYYY-MM-DD-hh-mm-ss)
- Timezone handling (JST +9 hours)
- Readable time string generation
- Sync state management
- Edge cases with time boundaries

### SDManager Logic Tests (11 tests)
- Filename parsing and timestamp extraction
- File validation (CSV, pressure_log_ prefix)
- CSV header and data line generation
- Legacy vs NTP filename handling
- Robustness with malformed inputs

### Data Processing Tests (11 tests)
- ADC to voltage conversion with voltage divider compensation
- Voltage to pressure calibration (1V=0MPa, 5V=1MPa)
- Output constraining to valid ranges (0-1 MPa)
- Buffer index management and wraparound
- End-to-end processing pipeline
- Precision and resolution validation

## Test Strategy

### What We Test
✅ **Pure Business Logic**: State machines, calculations, string processing
✅ **Data Transformations**: ADC conversion, pressure calculations, formatting
✅ **Validation Logic**: Input validation, range checking, format validation
✅ **Edge Cases**: Boundary conditions, invalid inputs, overflow scenarios

### What We Don't Test
❌ **Hardware Integration**: M5Stack LCD, SD card I/O, WiFi, sensors
❌ **Network Communication**: MQTT publishing, AWS IoT, NTP sync
❌ **File System Operations**: Actual file creation, deletion, reading
❌ **UI Rendering**: Display drawing, button detection, graphics

## Mock Strategy

Tests use lightweight mocks and test doubles:
- **Mock Arduino String**: Standard library implementation for string operations
- **Mock Hardware Classes**: Simple stubs that return predictable values
- **Extracted Logic**: Pure functions extracted from hardware-dependent classes
- **Dependency Injection**: Where possible, dependencies are injected for testing

## Test Principles

1. **Fast**: Tests run quickly without hardware dependencies
2. **Isolated**: Each test is independent and doesn't affect others
3. **Deterministic**: Tests produce consistent results across runs
4. **Comprehensive**: Cover normal cases, edge cases, and error conditions
5. **Readable**: Test names clearly describe expected behavior

## Adding New Tests

When adding new business logic:

1. **Extract Pure Functions**: Separate business logic from hardware dependencies
2. **Create Test File**: Follow naming convention `test_[component].cpp`
3. **Use Unity Framework**: Follow existing patterns for assertions
4. **Test Edge Cases**: Include boundary conditions and error scenarios
5. **Update Documentation**: Add test description to this README

## Common Test Patterns

```cpp
void test_function_should_behavior_when_condition(void) {
    // Arrange
    setup_test_data();
    
    // Act
    result = function_under_test(input);
    
    // Assert
    TEST_ASSERT_EQUAL(expected, result);
}
```

## Known Limitations

1. **Type Incompatibility**: Mock classes don't perfectly match Arduino types
2. **Hardware Simulation**: Cannot test actual hardware interactions
3. **Integration Gaps**: Limited testing of component interactions
4. **Platform Differences**: Native platform vs Arduino differences

These limitations are addressed through:
- Integration tests on actual hardware
- Hardware-in-the-loop testing for critical paths
- Manual testing of UI and hardware features

## Future Improvements

- [ ] Add integration tests with hardware abstraction layers
- [ ] Improve mock type compatibility with Arduino libraries
- [ ] Add performance benchmarks for critical algorithms
- [ ] Implement property-based testing for data conversion functions
- [ ] Add mutation testing to verify test quality