#ifdef UNIT_TEST

#include <unity.h>
#include <iostream>
#include <string>
#include "test_helpers.h"

// Define the SystemState enum for testing (copied from StateManager.h)
enum SystemState {
  STANDBY,
  RECORDING,
  FILE_LIST
};

// Create a testable version of StateManager that doesn't depend on Arduino libraries
class TestableStateManager {
private:
  SystemState currentState;
  unsigned long stateChangeTime;
  
public:
  TestableStateManager() : currentState(STANDBY), stateChangeTime(0) {}
  
  SystemState getCurrentState() {
    return currentState;
  }
  
  void transitionToStandby() {
    if (currentState != STANDBY) {
      currentState = STANDBY;
      stateChangeTime = millis();
      onEnterStandby();
    }
  }
  
  void transitionToRecording() {
    if (currentState != RECORDING) {
      currentState = RECORDING;
      stateChangeTime = millis();
      onEnterRecording();
    }
  }
  
  void transitionToFileList() {
    if (currentState != FILE_LIST) {
      currentState = FILE_LIST;
      stateChangeTime = millis();
      onEnterFileList();
    }
  }
  
  void toggleState() {
    if (currentState == STANDBY) {
      transitionToRecording();
    } else if (currentState == RECORDING) {
      transitionToStandby();
    } else if (currentState == FILE_LIST) {
      transitionToStandby();
    }
  }
  
  void handleButtonB() {
    if (currentState == STANDBY) {
      transitionToFileList();
    } else if (currentState == FILE_LIST) {
      transitionToStandby();
    }
    // Button B does nothing in RECORDING state to prevent accidental interruption
  }
  
private:
  void onEnterStandby() {
    // Mock implementation
  }
  
  void onEnterRecording() {
    // Mock implementation
  }
  
  void onEnterFileList() {
    // Mock implementation
  }
};

// Global test variables
TestableStateManager* stateManager;

void setUp(void) {
    stateManager = new TestableStateManager();
}

void tearDown(void) {
    delete stateManager;
}

void test_initial_state_should_be_standby(void) {
    TEST_ASSERT_EQUAL(STANDBY, stateManager->getCurrentState());
}

void test_toggle_state_from_standby_should_go_to_recording(void) {
    TEST_ASSERT_EQUAL(STANDBY, stateManager->getCurrentState());
    
    stateManager->toggleState();
    
    TEST_ASSERT_EQUAL(RECORDING, stateManager->getCurrentState());
}

void test_toggle_state_from_recording_should_go_to_standby(void) {
    stateManager->transitionToRecording();
    TEST_ASSERT_EQUAL(RECORDING, stateManager->getCurrentState());
    
    stateManager->toggleState();
    
    TEST_ASSERT_EQUAL(STANDBY, stateManager->getCurrentState());
}

void test_toggle_state_from_file_list_should_go_to_standby(void) {
    stateManager->transitionToFileList();
    TEST_ASSERT_EQUAL(FILE_LIST, stateManager->getCurrentState());
    
    stateManager->toggleState();
    
    TEST_ASSERT_EQUAL(STANDBY, stateManager->getCurrentState());
}

void test_handle_button_b_from_standby_should_go_to_file_list(void) {
    TEST_ASSERT_EQUAL(STANDBY, stateManager->getCurrentState());
    
    stateManager->handleButtonB();
    
    TEST_ASSERT_EQUAL(FILE_LIST, stateManager->getCurrentState());
}

void test_handle_button_b_from_file_list_should_go_to_standby(void) {
    stateManager->transitionToFileList();
    TEST_ASSERT_EQUAL(FILE_LIST, stateManager->getCurrentState());
    
    stateManager->handleButtonB();
    
    TEST_ASSERT_EQUAL(STANDBY, stateManager->getCurrentState());
}

void test_handle_button_b_from_recording_should_stay_recording(void) {
    stateManager->transitionToRecording();
    TEST_ASSERT_EQUAL(RECORDING, stateManager->getCurrentState());
    
    stateManager->handleButtonB();
    
    // Button B should do nothing in RECORDING state
    TEST_ASSERT_EQUAL(RECORDING, stateManager->getCurrentState());
}

void test_transition_to_standby_multiple_times_should_stay_standby(void) {
    stateManager->transitionToStandby();
    TEST_ASSERT_EQUAL(STANDBY, stateManager->getCurrentState());
    
    stateManager->transitionToStandby();
    TEST_ASSERT_EQUAL(STANDBY, stateManager->getCurrentState());
}

void test_transition_to_recording_multiple_times_should_stay_recording(void) {
    stateManager->transitionToRecording();
    TEST_ASSERT_EQUAL(RECORDING, stateManager->getCurrentState());
    
    stateManager->transitionToRecording();
    TEST_ASSERT_EQUAL(RECORDING, stateManager->getCurrentState());
}

void test_transition_to_file_list_multiple_times_should_stay_file_list(void) {
    stateManager->transitionToFileList();
    TEST_ASSERT_EQUAL(FILE_LIST, stateManager->getCurrentState());
    
    stateManager->transitionToFileList();
    TEST_ASSERT_EQUAL(FILE_LIST, stateManager->getCurrentState());
}

void test_all_state_transitions(void) {
    // Test complete state transition cycle
    TEST_ASSERT_EQUAL(STANDBY, stateManager->getCurrentState());
    
    // STANDBY -> RECORDING
    stateManager->transitionToRecording();
    TEST_ASSERT_EQUAL(RECORDING, stateManager->getCurrentState());
    
    // RECORDING -> FILE_LIST
    stateManager->transitionToFileList();
    TEST_ASSERT_EQUAL(FILE_LIST, stateManager->getCurrentState());
    
    // FILE_LIST -> STANDBY
    stateManager->transitionToStandby();
    TEST_ASSERT_EQUAL(STANDBY, stateManager->getCurrentState());
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    
    RUN_TEST(test_initial_state_should_be_standby);
    RUN_TEST(test_toggle_state_from_standby_should_go_to_recording);
    RUN_TEST(test_toggle_state_from_recording_should_go_to_standby);
    RUN_TEST(test_toggle_state_from_file_list_should_go_to_standby);
    RUN_TEST(test_handle_button_b_from_standby_should_go_to_file_list);
    RUN_TEST(test_handle_button_b_from_file_list_should_go_to_standby);
    RUN_TEST(test_handle_button_b_from_recording_should_stay_recording);
    RUN_TEST(test_transition_to_standby_multiple_times_should_stay_standby);
    RUN_TEST(test_transition_to_recording_multiple_times_should_stay_recording);
    RUN_TEST(test_transition_to_file_list_multiple_times_should_stay_file_list);
    RUN_TEST(test_all_state_transitions);
    
    return UNITY_END();
}

#endif // UNIT_TEST