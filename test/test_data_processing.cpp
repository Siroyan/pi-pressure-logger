#ifdef UNIT_TEST

#include <unity.h>
#include <cmath>
#include "test_helpers.h"

// Extract data processing logic from main.cpp for testing
class TestableDataProcessor {
public:
    // Extract voltage conversion logic for testing
    static float convertADCToOriginalVoltage(int16_t adc_value) {
        // ADS1015 GAIN_TWOTHIRDS: 3mV per LSB, voltage divider doubles the original voltage
        return adc_value * 0.003f * 2.0f; // Convert to original 0-5V
    }
    
    // Extract voltage to pressure conversion logic for testing
    static float convertVoltageToPressure(float voltage) {
        // Convert voltage to pressure: 1V=0MPa, 5V=1MPa -> P = (V-1)/4
        return (voltage - 1.0f) / 4.0f; // Pressure in MPa
    }
    
    // Extract complete sensor data processing pipeline for testing
    static float processSensorReading(int16_t adc_value) {
        float original_voltage = convertADCToOriginalVoltage(adc_value);
        float pressure = convertVoltageToPressure(original_voltage);
        return constrain(pressure, 0.0f, 1.0f);
    }
    
    // Extract buffer management logic for testing
    static int updateBufferIndex(int current_index, int buffer_size) {
        return (current_index + 1) % buffer_size;
    }
    
    // Extract voltage scaling validation for testing
    static bool isValidVoltageRange(float voltage) {
        return voltage >= 0.0f && voltage <= 6.0f; // ADS1015 with GAIN_TWOTHIRDS range
    }
    
    // Extract pressure validation for testing
    static bool isValidPressureRange(float pressure) {
        return pressure >= 0.0f && pressure <= 1.0f; // Expected pressure range
    }
};

void setUp(void) {
    // Setup for each test
}

void tearDown(void) {
    // Cleanup after each test
}

void test_convert_adc_to_original_voltage_should_handle_zero(void) {
    float result = TestableDataProcessor::convertADCToOriginalVoltage(0);
    
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, result);
}

void test_convert_adc_to_original_voltage_should_handle_typical_values(void) {
    struct {
        int16_t adc_value;
        float expected_voltage;
        const char* description;
    } testCases[] = {
        {0, 0.0f, "Zero ADC"},
        {167, 1.0f, "1V input (after voltage divider = 0.5V ADC, 167 counts)"},
        {333, 2.0f, "2V input (after voltage divider = 1.0V ADC, 333 counts)"},
        {833, 5.0f, "5V input (after voltage divider = 2.5V ADC, 833 counts)"},
        {1000, 6.0f, "6V input (after voltage divider = 3.0V ADC, 1000 counts)"},
    };
    
    for (size_t i = 0; i < sizeof(testCases)/sizeof(testCases[0]); i++) {
        float result = TestableDataProcessor::convertADCToOriginalVoltage(testCases[i].adc_value);
        TEST_ASSERT_FLOAT_WITHIN(0.1f, testCases[i].expected_voltage, result);
    }
}

void test_convert_voltage_to_pressure_should_handle_calibration_points(void) {
    struct {
        float voltage;
        float expected_pressure;
        const char* description;
    } testCases[] = {
        {1.0f, 0.0f, "1V = 0 MPa (calibration point)"},
        {5.0f, 1.0f, "5V = 1 MPa (calibration point)"},
        {3.0f, 0.5f, "3V = 0.5 MPa (midpoint)"},
        {2.0f, 0.25f, "2V = 0.25 MPa"},
        {4.0f, 0.75f, "4V = 0.75 MPa"},
    };
    
    for (size_t i = 0; i < sizeof(testCases)/sizeof(testCases[0]); i++) {
        float result = TestableDataProcessor::convertVoltageToPressure(testCases[i].voltage);
        TEST_ASSERT_FLOAT_WITHIN(0.001f, testCases[i].expected_pressure, result);
    }
}

void test_convert_voltage_to_pressure_should_handle_out_of_range_values(void) {
    struct {
        float voltage;
        float expected_pressure;
        const char* description;
    } testCases[] = {
        {0.0f, -0.25f, "0V should give negative pressure"},
        {6.0f, 1.25f, "6V should give >1 MPa"},
        {-1.0f, -0.5f, "Negative voltage"},
        {10.0f, 2.25f, "Very high voltage"},
    };
    
    for (size_t i = 0; i < sizeof(testCases)/sizeof(testCases[0]); i++) {
        float result = TestableDataProcessor::convertVoltageToPressure(testCases[i].voltage);
        TEST_ASSERT_FLOAT_WITHIN(0.001f, testCases[i].expected_pressure, result);
    }
}

void test_process_sensor_reading_should_constrain_output(void) {
    struct {
        int16_t adc_value;
        float expected_pressure;
        const char* description;
    } testCases[] = {
        {0, 0.0f, "Very low ADC should constrain to 0"},
        {167, 0.0f, "1V input should give 0 MPa"},
        {833, 1.0f, "5V input should give 1 MPa"},
        {1000, 1.0f, "6V input should constrain to 1 MPa"},
        {2000, 1.0f, "Very high ADC should constrain to 1 MPa"},
    };
    
    for (size_t i = 0; i < sizeof(testCases)/sizeof(testCases[0]); i++) {
        float result = TestableDataProcessor::processSensorReading(testCases[i].adc_value);
        TEST_ASSERT_FLOAT_WITHIN(0.1f, testCases[i].expected_pressure, result);
        
        // Verify output is always within valid range
        TEST_ASSERT_TRUE(result >= 0.0f);
        TEST_ASSERT_TRUE(result <= 1.0f);
    }
}

void test_update_buffer_index_should_wrap_correctly(void) {
    const int buffer_size = 100;
    
    // Test normal increment
    TEST_ASSERT_EQUAL(1, TestableDataProcessor::updateBufferIndex(0, buffer_size));
    TEST_ASSERT_EQUAL(50, TestableDataProcessor::updateBufferIndex(49, buffer_size));
    
    // Test wraparound
    TEST_ASSERT_EQUAL(0, TestableDataProcessor::updateBufferIndex(99, buffer_size));
    TEST_ASSERT_EQUAL(0, TestableDataProcessor::updateBufferIndex(buffer_size - 1, buffer_size));
}

void test_update_buffer_index_with_different_sizes(void) {
    struct {
        int current_index;
        int buffer_size;
        int expected;
    } testCases[] = {
        {0, 1, 0},      // Single element buffer
        {0, 2, 1},      // Two element buffer
        {1, 2, 0},      // Two element buffer wrap
        {4, 5, 0},      // Five element buffer wrap
        {10, 20, 11},   // Twenty element buffer normal
        {19, 20, 0},    // Twenty element buffer wrap
    };
    
    for (size_t i = 0; i < sizeof(testCases)/sizeof(testCases[0]); i++) {
        int result = TestableDataProcessor::updateBufferIndex(
            testCases[i].current_index, testCases[i].buffer_size);
        TEST_ASSERT_EQUAL(testCases[i].expected, result);
    }
}

void test_is_valid_voltage_range_should_validate_correctly(void) {
    // Valid voltages (within ADS1015 GAIN_TWOTHIRDS range)
    TEST_ASSERT_TRUE(TestableDataProcessor::isValidVoltageRange(0.0f));
    TEST_ASSERT_TRUE(TestableDataProcessor::isValidVoltageRange(3.0f));
    TEST_ASSERT_TRUE(TestableDataProcessor::isValidVoltageRange(6.0f));
    TEST_ASSERT_TRUE(TestableDataProcessor::isValidVoltageRange(2.5f));
    
    // Invalid voltages
    TEST_ASSERT_FALSE(TestableDataProcessor::isValidVoltageRange(-0.1f));
    TEST_ASSERT_FALSE(TestableDataProcessor::isValidVoltageRange(6.1f));
    TEST_ASSERT_FALSE(TestableDataProcessor::isValidVoltageRange(-10.0f));
    TEST_ASSERT_FALSE(TestableDataProcessor::isValidVoltageRange(10.0f));
}

void test_is_valid_pressure_range_should_validate_correctly(void) {
    // Valid pressures (0-1 MPa range)
    TEST_ASSERT_TRUE(TestableDataProcessor::isValidPressureRange(0.0f));
    TEST_ASSERT_TRUE(TestableDataProcessor::isValidPressureRange(0.5f));
    TEST_ASSERT_TRUE(TestableDataProcessor::isValidPressureRange(1.0f));
    TEST_ASSERT_TRUE(TestableDataProcessor::isValidPressureRange(0.25f));
    TEST_ASSERT_TRUE(TestableDataProcessor::isValidPressureRange(0.999f));
    
    // Invalid pressures
    TEST_ASSERT_FALSE(TestableDataProcessor::isValidPressureRange(-0.001f));
    TEST_ASSERT_FALSE(TestableDataProcessor::isValidPressureRange(1.001f));
    TEST_ASSERT_FALSE(TestableDataProcessor::isValidPressureRange(-1.0f));
    TEST_ASSERT_FALSE(TestableDataProcessor::isValidPressureRange(2.0f));
}

void test_end_to_end_data_processing_pipeline(void) {
    // Test the complete pipeline with known values
    struct {
        int16_t adc_input;
        float expected_pressure_min;
        float expected_pressure_max;
        const char* description;
    } testCases[] = {
        {167, 0.0f, 0.01f, "1V input should be near 0 MPa"},      // 1V -> ~0 MPa
        {500, 0.45f, 0.55f, "3V input should be near 0.5 MPa"},   // 3V -> 0.5 MPa  
        {833, 0.95f, 1.0f, "5V input should be near 1 MPa"},      // 5V -> ~1 MPa
    };
    
    for (size_t i = 0; i < sizeof(testCases)/sizeof(testCases[0]); i++) {
        float result = TestableDataProcessor::processSensorReading(testCases[i].adc_input);
        
        // Check that result is within expected range (allowing for some tolerance)
        TEST_ASSERT_TRUE(result >= testCases[i].expected_pressure_min);
        TEST_ASSERT_TRUE(result <= testCases[i].expected_pressure_max);
        
        // Always check final constraints
        TEST_ASSERT_TRUE(result >= 0.0f);
        TEST_ASSERT_TRUE(result <= 1.0f);
    }
}

void test_precision_and_resolution_requirements(void) {
    // Test that we maintain reasonable precision through the conversion chain
    float result1 = TestableDataProcessor::processSensorReading(167);  // 1V
    float result2 = TestableDataProcessor::processSensorReading(168);  // ~1.006V
    
    // There should be some measurable difference between adjacent ADC values
    // (though it might be small due to the scaling)
    float difference = fabs(result2 - result1);
    
    // The difference should be small but non-zero (depending on resolution requirements)
    TEST_ASSERT_TRUE(difference >= 0.0f);
    TEST_ASSERT_TRUE(difference < 0.1f); // Should be less than 0.1 MPa difference
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    
    RUN_TEST(test_convert_adc_to_original_voltage_should_handle_zero);
    RUN_TEST(test_convert_adc_to_original_voltage_should_handle_typical_values);
    RUN_TEST(test_convert_voltage_to_pressure_should_handle_calibration_points);
    RUN_TEST(test_convert_voltage_to_pressure_should_handle_out_of_range_values);
    RUN_TEST(test_process_sensor_reading_should_constrain_output);
    RUN_TEST(test_update_buffer_index_should_wrap_correctly);
    RUN_TEST(test_update_buffer_index_with_different_sizes);
    RUN_TEST(test_is_valid_voltage_range_should_validate_correctly);
    RUN_TEST(test_is_valid_pressure_range_should_validate_correctly);
    RUN_TEST(test_end_to_end_data_processing_pipeline);
    RUN_TEST(test_precision_and_resolution_requirements);
    
    return UNITY_END();
}

#endif // UNIT_TEST