#pragma once
#include <M5Stack.h>
#include "AcquisitionService.h"
#include "StateManager.h"
#include "NetworkService.h"
#include "StorageService.h"
#include "DisplayManager.h"

// Stable lifetime owner. Each task receives this object through its parameter;
// only tick() owns screen, buttons, navigation and user-visible state transitions.
class LoggerApplication {
  static constexpr int interval_ms=10, sample_queue_size=512, max_samples_per_loop=4;
  static constexpr int min_press_duration=50;
  RecordingQueue recordingQueue;
  AcquisitionService acquisitionService{recordingQueue};
  NetworkService networkService{recordingQueue};
  SDManager sdManager;
  StorageService storageService{sdManager,recordingQueue};
  DisplayManager displayManager;
  StateManager stateManager{recordingQueue};
  bool storageTaskReady=false;
  const char* startupError=nullptr;
  FileAction fileAction;
  std::vector<String> listedFiles;
  std::vector<long> listedSizes;
  bool fileRequestPending=false, screenDirty=true, fileOperationSuccess=true;
  uint32_t button_press_time=0, last_status_update=0, last_drop_report=0;
  void requestFileList();
  void handleButtonInput();
  static void samplingTask(void*);
  static void storageTask(void*);
  static void networkTask(void*);
public:
  void setup();
  void tick();
  void sample() { acquisitionService.step(); }
  void store() { storageService.step(); }
  void network() { networkService.step(); }
  SystemState currentState() const { return stateManager.getCurrentState(); }
};
