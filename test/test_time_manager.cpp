#ifdef UNIT_TEST

#include <unity.h>
#include <ctime>
#include <string>
#include "test_helpers.h"

// We'll use the String class from test_helpers.h instead of redefining it

class TestableTimeManager {
private:
    bool time_synced;
    const char* timezone_name;
    long timezone_offset_seconds;
    
public:
    TestableTimeManager(const char* tz_name = "JST", long tz_offset = 32400) 
        : time_synced(false), timezone_name(tz_name), timezone_offset_seconds(tz_offset) {}
    
    void setTimeSynced(bool synced) { time_synced = synced; }
    bool isTimeSynced() { return time_synced; }
    
    // Test version that takes a timestamp parameter
    String getFormattedTimeString(time_t timestamp) {
        if (!time_synced) return String("");
        
        // Apply timezone offset
        timestamp += timezone_offset_seconds;
        
        struct tm* timeinfo = gmtime(&timestamp);
        if (!timeinfo) return String("");
        
        char buffer[20];
        snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d-%02d-%02d-%02d",
                timeinfo->tm_year + 1900,
                timeinfo->tm_mon + 1,
                timeinfo->tm_mday,
                timeinfo->tm_hour,
                timeinfo->tm_min,
                timeinfo->tm_sec);
        
        return String(buffer);
    }
    
    String getCurrentTimeString(time_t timestamp) {
        if (!time_synced) return String("Time not synced");
        
        timestamp += timezone_offset_seconds;
        struct tm* timeinfo = gmtime(&timestamp);
        if (!timeinfo) return String("Invalid time");
        
        char buffer[30];
        snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d %02d:%02d:%02d %s",
                timeinfo->tm_year + 1900,
                timeinfo->tm_mon + 1,
                timeinfo->tm_mday,
                timeinfo->tm_hour,
                timeinfo->tm_min,
                timeinfo->tm_sec,
                timezone_name);
        
        return String(buffer);
    }
};

// Test fixtures
TestableTimeManager* timeManager;

void setUp(void) {
    timeManager = new TestableTimeManager();
}

void tearDown(void) {
    delete timeManager;
}

void test_initial_state_should_not_be_synced(void) {
    TEST_ASSERT_FALSE(timeManager->isTimeSynced());
}

void test_formatted_time_string_when_not_synced_should_return_empty(void) {
    timeManager->setTimeSynced(false);
    
    String result = timeManager->getFormattedTimeString(1640995200); // 2022-01-01 00:00:00 UTC
    
    TEST_ASSERT_EQUAL_STRING("", result.c_str());
}

void test_formatted_time_string_when_synced_should_return_correct_format(void) {
    timeManager->setTimeSynced(true);
    
    // Test with known timestamp: 2022-01-01 00:00:00 UTC
    // With JST offset (+9 hours), this becomes 2022-01-01 09:00:00 JST
    String result = timeManager->getFormattedTimeString(1640995200);
    
    TEST_ASSERT_EQUAL_STRING("2022-01-01-09-00-00", result.c_str());
}

void test_formatted_time_string_with_different_times(void) {
    timeManager->setTimeSynced(true);
    
    // Test multiple timestamps
    struct {
        time_t timestamp;
        const char* expected;
    } testCases[] = {
        {1640995200, "2022-01-01-09-00-00"}, // 2022-01-01 00:00:00 UTC -> 09:00:00 JST
        {1641081600, "2022-01-02-09-00-00"}, // 2022-01-02 00:00:00 UTC -> 09:00:00 JST
        {1640998800, "2022-01-01-10-00-00"}, // 2022-01-01 01:00:00 UTC -> 10:00:00 JST
    };
    
    for (size_t i = 0; i < sizeof(testCases)/sizeof(testCases[0]); i++) {
        String result = timeManager->getFormattedTimeString(testCases[i].timestamp);
        TEST_ASSERT_EQUAL_STRING(testCases[i].expected, result.c_str());
    }
}

void test_current_time_string_when_not_synced_should_return_error_message(void) {
    timeManager->setTimeSynced(false);
    
    String result = timeManager->getCurrentTimeString(1640995200);
    
    TEST_ASSERT_EQUAL_STRING("Time not synced", result.c_str());
}

void test_current_time_string_when_synced_should_return_readable_format(void) {
    timeManager->setTimeSynced(true);
    
    // Test with known timestamp: 2022-01-01 00:00:00 UTC
    // With JST offset (+9 hours), this becomes 2022-01-01 09:00:00 JST
    String result = timeManager->getCurrentTimeString(1640995200);
    
    TEST_ASSERT_EQUAL_STRING("2022-01-01 09:00:00 JST", result.c_str());
}

void test_timezone_handling_with_different_offsets(void) {
    // Test with UTC timezone (offset 0)
    TestableTimeManager utcManager("UTC", 0);
    utcManager.setTimeSynced(true);
    
    String result = utcManager.getFormattedTimeString(1640995200);
    TEST_ASSERT_EQUAL_STRING("2022-01-01-00-00-00", result.c_str());
    
    String currentResult = utcManager.getCurrentTimeString(1640995200);
    TEST_ASSERT_EQUAL_STRING("2022-01-01 00:00:00 UTC", currentResult.c_str());
}

void test_edge_cases_with_time_boundaries(void) {
    timeManager->setTimeSynced(true);
    
    // Test end of year/month boundaries
    struct {
        time_t timestamp;
        const char* expected;
        const char* description;
    } testCases[] = {
        {1640908800, "2021-12-31-09-00-00"}, // 2021-12-31 00:00:00 UTC -> 2021-12-31 09:00:00 JST
        {1640995200, "2022-01-01-09-00-00"}, // 2022-01-01 00:00:00 UTC -> 2022-01-01 09:00:00 JST
        {1643673599, "2022-02-01-08-59-59"}, // 2022-01-31 23:59:59 UTC -> 2022-02-01 08:59:59 JST
    };
    
    for (size_t i = 0; i < sizeof(testCases)/sizeof(testCases[0]); i++) {
        String result = timeManager->getFormattedTimeString(testCases[i].timestamp);
        TEST_ASSERT_EQUAL_STRING(testCases[i].expected, result.c_str());
    }
}

void test_sync_state_changes(void) {
    // Test that sync state affects output correctly
    TEST_ASSERT_FALSE(timeManager->isTimeSynced());
    
    String result1 = timeManager->getFormattedTimeString(1640995200);
    TEST_ASSERT_EQUAL_STRING("", result1.c_str());
    
    timeManager->setTimeSynced(true);
    TEST_ASSERT_TRUE(timeManager->isTimeSynced());
    
    String result2 = timeManager->getFormattedTimeString(1640995200);
    TEST_ASSERT_EQUAL_STRING("2022-01-01-09-00-00", result2.c_str());
    
    timeManager->setTimeSynced(false);
    TEST_ASSERT_FALSE(timeManager->isTimeSynced());
    
    String result3 = timeManager->getFormattedTimeString(1640995200);
    TEST_ASSERT_EQUAL_STRING("", result3.c_str());
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    
    RUN_TEST(test_initial_state_should_not_be_synced);
    RUN_TEST(test_formatted_time_string_when_not_synced_should_return_empty);
    RUN_TEST(test_formatted_time_string_when_synced_should_return_correct_format);
    RUN_TEST(test_formatted_time_string_with_different_times);
    RUN_TEST(test_current_time_string_when_not_synced_should_return_error_message);
    RUN_TEST(test_current_time_string_when_synced_should_return_readable_format);
    RUN_TEST(test_timezone_handling_with_different_offsets);
    RUN_TEST(test_edge_cases_with_time_boundaries);
    RUN_TEST(test_sync_state_changes);
    
    return UNITY_END();
}

#endif // UNIT_TEST