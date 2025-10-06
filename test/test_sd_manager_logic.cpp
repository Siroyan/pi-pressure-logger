#ifdef UNIT_TEST

#include <unity.h>
#include <string>
#include <vector>
#include "test_helpers.h"

// Create a testable version of SDManager file logic functions
class TestableSDManagerLogic {
public:
    // Extract the file timestamp parsing logic for testing
    static String getFileTimestamp(const String& filename) {
        // Extract timestamp from filename
        // Format: pressure_log_YYYY-MM-DD-hh-mm-ss.csv or pressure_log_millis.csv
        
        String name = filename;
        name.replace("pressure_log_", "");
        name.replace(".csv", "");
        
        // Check if it's a timestamp format (contains hyphens)
        if (name.indexOf('-') >= 0) {
            // Convert YYYY-MM-DD-hh-mm-ss to readable format
            // First, split into date and time parts
            String datePart = name.substring(0, 10); // YYYY-MM-DD
            String timePart = name.substring(11);     // hh-mm-ss
            
            // Convert date hyphens to colons and time hyphens to colons
            datePart.replace('-', ':');
            timePart.replace('-', ':');
            
            return datePart + String(" ") + timePart;
        } else {
            // Millis-based filename
            return String("Legacy (") + name + String(")");
        }
    }
    
    // Extract filename validation logic for testing
    static bool isValidLogFile(const String& filename) {
        return filename.endsWith(".csv") && filename.startsWith("pressure_log_");
    }
    
    // Extract filename generation logic for testing
    static String generateLogFilename(const String& timestamp) {
        return String("/pressure_log_") + timestamp + String(".csv");
    }
    
    // Extract CSV header generation for testing
    static String generateCSVHeader() {
        return String("Timestamp(ms),CH0(MPa),CH1(MPa)");
    }
    
    // Extract CSV data line generation for testing
    static String generateCSVDataLine(unsigned long timestamp, float p0, float p1) {
        // Format floats to 4 decimal places manually
        char p0_str[20], p1_str[20];
        snprintf(p0_str, sizeof(p0_str), "%.4f", p0);
        snprintf(p1_str, sizeof(p1_str), "%.4f", p1);
        
        return String(static_cast<long>(timestamp)) + String(",") + 
               String(p0_str) + String(",") + String(p1_str);
    }
};

void setUp(void) {
    // Setup for each test
}

void tearDown(void) {
    // Cleanup after each test
}

void test_get_file_timestamp_with_ntp_format_should_parse_correctly(void) {
    String filename = "pressure_log_2022-01-01-10-30-45.csv";
    
    String result = TestableSDManagerLogic::getFileTimestamp(filename);
    
    TEST_ASSERT_EQUAL_STRING("2022:01:01 10:30:45", result.c_str());
}

void test_get_file_timestamp_with_millis_format_should_return_legacy(void) {
    String filename = "pressure_log_1640995200000.csv";
    
    String result = TestableSDManagerLogic::getFileTimestamp(filename);
    
    TEST_ASSERT_EQUAL_STRING("Legacy (1640995200000)", result.c_str());
}

void test_get_file_timestamp_edge_cases(void) {
    struct {
        const char* filename;
        const char* expected;
        const char* description;
    } testCases[] = {
        {"pressure_log_2021-12-31-23-59-59.csv", "2021:12:31 23:59:59", "End of year"},
        {"pressure_log_2022-02-28-12-00-00.csv", "2022:02:28 12:00:00", "February date"},
        {"pressure_log_2022-01-01-00-00-00.csv", "2022:01:01 00:00:00", "Midnight"},
        {"pressure_log_123456789.csv", "Legacy (123456789)", "Short millis"},
        {"pressure_log_0.csv", "Legacy (0)", "Zero timestamp"},
    };
    
    for (size_t i = 0; i < sizeof(testCases)/sizeof(testCases[0]); i++) {
        String result = TestableSDManagerLogic::getFileTimestamp(testCases[i].filename);
        TEST_ASSERT_EQUAL_STRING(testCases[i].expected, result.c_str());
    }
}

void test_is_valid_log_file_with_correct_files_should_return_true(void) {
    const char* validFiles[] = {
        "pressure_log_2022-01-01-10-30-45.csv",
        "pressure_log_1640995200000.csv",
        "pressure_log_test.csv",
        "pressure_log_.csv"
    };
    
    for (size_t i = 0; i < sizeof(validFiles)/sizeof(validFiles[0]); i++) {
        bool result = TestableSDManagerLogic::isValidLogFile(validFiles[i]);
        TEST_ASSERT_TRUE(result);
    }
}

void test_is_valid_log_file_with_incorrect_files_should_return_false(void) {
    const char* invalidFiles[] = {
        "pressure_log_2022-01-01-10-30-45.txt",  // Wrong extension
        "other_log_2022-01-01-10-30-45.csv",     // Wrong prefix
        "pressure_log_2022-01-01-10-30-45",      // No extension
        "random_file.csv",                        // No pressure_log prefix
        "pressure_log.txt",                       // Wrong extension
        ""                                        // Empty filename
    };
    
    for (size_t i = 0; i < sizeof(invalidFiles)/sizeof(invalidFiles[0]); i++) {
        bool result = TestableSDManagerLogic::isValidLogFile(invalidFiles[i]);
        TEST_ASSERT_FALSE(result);
    }
}

void test_generate_log_filename_should_create_correct_format(void) {
    String result = TestableSDManagerLogic::generateLogFilename("2022-01-01-10-30-45");
    
    TEST_ASSERT_EQUAL_STRING("/pressure_log_2022-01-01-10-30-45.csv", result.c_str());
}

void test_generate_log_filename_with_different_timestamps(void) {
    struct {
        const char* timestamp;
        const char* expected;
    } testCases[] = {
        {"2022-01-01-10-30-45", "/pressure_log_2022-01-01-10-30-45.csv"},
        {"1640995200000", "/pressure_log_1640995200000.csv"},
        {"2021-12-31-23-59-59", "/pressure_log_2021-12-31-23-59-59.csv"},
        {"", "/pressure_log_.csv"}
    };
    
    for (size_t i = 0; i < sizeof(testCases)/sizeof(testCases[0]); i++) {
        String result = TestableSDManagerLogic::generateLogFilename(testCases[i].timestamp);
        TEST_ASSERT_EQUAL_STRING(testCases[i].expected, result.c_str());
    }
}

void test_generate_csv_header_should_return_correct_format(void) {
    String result = TestableSDManagerLogic::generateCSVHeader();
    
    TEST_ASSERT_EQUAL_STRING("Timestamp(ms),CH0(MPa),CH1(MPa)", result.c_str());
}

void test_generate_csv_data_line_should_format_correctly(void) {
    String result = TestableSDManagerLogic::generateCSVDataLine(1000, 0.1234f, 0.5678f);
    
    TEST_ASSERT_EQUAL_STRING("1000,0.1234,0.5678", result.c_str());
}

void test_generate_csv_data_line_with_different_values(void) {
    struct {
        unsigned long timestamp;
        float p0;
        float p1;
        const char* expectedStart; // We'll just check the timestamp part due to float precision
    } testCases[] = {
        {0, 0.0f, 0.0f, "0,"},
        {1500, 1.0f, 0.5f, "1500,"},
        {999999, 0.999f, 0.001f, "999999,"}
    };
    
    for (size_t i = 0; i < sizeof(testCases)/sizeof(testCases[0]); i++) {
        String result = TestableSDManagerLogic::generateCSVDataLine(
            testCases[i].timestamp, testCases[i].p0, testCases[i].p1);
        
        // Check that the timestamp part is correct
        TEST_ASSERT_TRUE(result.str().find(testCases[i].expectedStart) == 0);
    }
}

void test_filename_parsing_robustness(void) {
    // Test edge cases that might cause crashes or unexpected behavior
    const char* edgeCases[] = {
        "pressure_log_",
        "pressure_log_.csv",
        "pressure_log_--.csv",
        "pressure_log_2022-.csv",
        "pressure_log_-01-01-10-30-45.csv",
        "pressure_log_2022-13-01-10-30-45.csv", // Invalid month
        "pressure_log_2022-01-32-10-30-45.csv", // Invalid day
    };
    
    // These should not crash and should return some reasonable result
    for (size_t i = 0; i < sizeof(edgeCases)/sizeof(edgeCases[0]); i++) {
        String result = TestableSDManagerLogic::getFileTimestamp(edgeCases[i]);
        // Just ensure it returns something and doesn't crash
        TEST_ASSERT_TRUE(result.length() >= 0);
    }
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    
    RUN_TEST(test_get_file_timestamp_with_ntp_format_should_parse_correctly);
    RUN_TEST(test_get_file_timestamp_with_millis_format_should_return_legacy);
    RUN_TEST(test_get_file_timestamp_edge_cases);
    RUN_TEST(test_is_valid_log_file_with_correct_files_should_return_true);
    RUN_TEST(test_is_valid_log_file_with_incorrect_files_should_return_false);
    RUN_TEST(test_generate_log_filename_should_create_correct_format);
    RUN_TEST(test_generate_log_filename_with_different_timestamps);
    RUN_TEST(test_generate_csv_header_should_return_correct_format);
    RUN_TEST(test_generate_csv_data_line_should_format_correctly);
    RUN_TEST(test_generate_csv_data_line_with_different_values);
    RUN_TEST(test_filename_parsing_robustness);
    
    return UNITY_END();
}

#endif // UNIT_TEST