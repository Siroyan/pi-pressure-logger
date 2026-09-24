#include "LoggerApplication.h"
#include <freertos/task.h>

void LoggerApplication::requestFileList() {
  fileRequestPending = storageTaskReady && storageService.requestList();
  fileOperationSuccess = fileRequestPending;
  screenDirty = true;
}

void LoggerApplication::handleButtonInput() {
  if (stateManager.getCurrentState() == FILE_LIST && fileAction.status() != FileAction::Status::None) {
    if (M5.BtnC.wasPressed() && fileAction.dismiss()) requestFileList();
    else if (M5.BtnB.wasPressed() && fileAction.confirm()) {
      fileRequestPending = storageTaskReady && storageService.requestDelete(fileAction.filename());
      if (!fileRequestPending) fileAction.complete(false);
      screenDirty = true;
    }
    button_press_time = 0;
    return;
  }
  if (M5.BtnA.wasPressed()) button_press_time = millis();
  if (M5.BtnA.wasReleased() && button_press_time > 0) {
    if (millis() - button_press_time >= min_press_duration) {
      if (stateManager.getCurrentState() == FILE_LIST) {
        if (!fileRequestPending) {
          displayManager.navigateFileList(1, listedFiles.size());
          screenDirty = true;
        }
      } else if (acquisitionService.isAvailable() || stateManager.getCurrentState() == RECORDING) {
        stateManager.toggleState();
        screenDirty = true;
      }
    }
    button_press_time = 0;
  }
  if (M5.BtnB.wasPressed()) {
    if (stateManager.getCurrentState() == FILE_LIST) {
      int selected = displayManager.getSelectedFileIndex();
      if (!fileRequestPending && selected >= 0 && selected < (int)listedFiles.size()) {
        fileAction.begin(listedFiles[selected],listedSizes[selected]);
        screenDirty = true;
      }
    } else if (stateManager.getCurrentState() == STANDBY) {
      stateManager.transitionToFileList();
      displayManager.resetFileListNavigation();
      requestFileList();
    }
  }
  if (M5.BtnC.wasPressed() && stateManager.getCurrentState() == FILE_LIST) {
    stateManager.transitionToStandby();
    screenDirty = true;
  }
}

void LoggerApplication::storageTask(void* parameter) {
  auto& app=*static_cast<LoggerApplication*>(parameter);
  while (true) {
    app.store();
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}


void LoggerApplication::samplingTask(void* parameter) {
  auto& app=*static_cast<LoggerApplication*>(parameter);
  TickType_t last_wake_time = xTaskGetTickCount();
  const TickType_t sample_period = pdMS_TO_TICKS(interval_ms);

  while (true) {
    vTaskDelayUntil(&last_wake_time, sample_period);

    app.sample();
    // タイムアウトやタスクの中断後に、遅れた分の変換を連続実行しない。
    if (xTaskGetTickCount() - last_wake_time >= sample_period) last_wake_time = xTaskGetTickCount();
  }
}

void LoggerApplication::networkTask(void* parameter) {
  auto& app=*static_cast<LoggerApplication*>(parameter);
  while (true) {
    app.network();
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void LoggerApplication::setup() {
  M5.begin(true,false); // SDのマウントと復旧時の設定はSDManagerが担当する。

  acquisitionService.init();

  displayManager.init();
  sdManager.init();
  networkService.init();

  // ログファイル名に使う時計をSDManagerへ渡す。
  sdManager.setTimeManager(networkService.timeSource());

  displayManager.drawConnectionStatus(acquisitionService.isAvailable(), sdManager.isAvailable(), sdManager.isRecording(),
                                      sdManager.hasWriteError(),
                                      networkService.wifiConnected(), networkService.mqttConnected(), networkService.enabled());

  // サンプルの受け手を先に用意するため、記録タスクを計測タスクより先に起動する。
  bool samplingTaskReady = false;
  if (!recordingQueue.init(sample_queue_size)) {
    startupError = "Sample queues unavailable";
  } else if (!storageService.init()) {
    startupError = "Storage resources unavailable";
  } else if (xTaskCreatePinnedToCore(storageTask,"pressure-storage",6144,this,1,nullptr,1)!=pdPASS) {
    startupError = "Storage task unavailable";
  } else {
    storageTaskReady = true;
    samplingTaskReady = xTaskCreatePinnedToCore(samplingTask,"pressure-sampling",4096,this,3,nullptr,1)==pdPASS;
    if (!samplingTaskReady) startupError = "Sampling task unavailable";
  }
  // 通信タスクは独立して起動する。記録・計測側の起動エラーがあれば優先して残す。
  const char* networkError = nullptr;
  if (!networkService.ready()) {
    networkError = "MQTT resources unavailable";
  } else if (networkService.enabled() &&
             xTaskCreatePinnedToCore(networkTask,"network-maintenance",8192,this,tskIDLE_PRIORITY,nullptr,0)!=pdPASS) {
    networkError = "Network task unavailable";
  }
  if (!startupError) startupError = networkError;
  stateManager.setRecordingReady(samplingTaskReady);
  if (startupError) Serial.println(startupError);

}

void LoggerApplication::tick() {
  M5.update();
  uint32_t now = millis();

  if (!acquisitionService.isAvailable() && stateManager.getCurrentState() == RECORDING) {
    stateManager.transitionToStandby();
    screenDirty = true;
  }
  handleButtonInput();

  if (storageService.takeResult(listedFiles, listedSizes, fileOperationSuccess)) {
    fileRequestPending = false;
    if (fileAction.status()==FileAction::Status::Busy) fileAction.complete(fileOperationSuccess);
    displayManager.resetFileListNavigation();
    screenDirty = true;
  }
  // SDと液晶はSPIを共有する。SD処理中は描画を見送り、ボタン入力を待たせない。
  if (!storageTaskReady || storageService.tryBeginDisplay()) {
    if (stateManager.getCurrentState() == FILE_LIST) {
      if (screenDirty) {
        if (fileAction.status()!=FileAction::Status::None) {
          displayManager.drawFileAction(fileAction);
        } else if (fileRequestPending) {
          M5.Lcd.fillScreen(BLACK); M5.Lcd.setCursor(10,10); M5.Lcd.print("Loading... C:Back");
        } else {
          displayManager.drawFileList(listedFiles, listedSizes);
          if (!fileOperationSuccess) { M5.Lcd.setCursor(10,200); M5.Lcd.print("Storage operation failed"); }
        }
        screenDirty = false;
      }
    } else {
      if (screenDirty) { displayManager.init(); screenDirty = false; }
      displayManager.advanceGraph(acquisitionMillis());
      if (now - last_status_update >= 500) {
        last_status_update = now;
        M5.Lcd.fillRect(30,200,280,9,BLACK);
        if (startupError) { M5.Lcd.setCursor(30,200); M5.Lcd.print(startupError); }
        displayManager.drawConnectionStatus(acquisitionService.isAvailable(), sdManager.isAvailable(), sdManager.isRecording(),
                                            sdManager.hasWriteError(),
                                            networkService.wifiConnected(), networkService.mqttConnected(), networkService.enabled());
      }
    }
    PressureSample sample;
    for (int i=0; i<max_samples_per_loop && recordingQueue.receiveDisplay(sample); ++i) {
      if (stateManager.getCurrentState()!=FILE_LIST) {
        displayManager.drawSample(sample.p0,sample.p1,sample.timestamp);
        displayManager.drawPressureText(sample.p0,sample.p1);
      }
    }
    if (storageTaskReady) storageService.endDisplay();
  }
  if (now - last_drop_report >= 5000) {
    last_drop_report = now;
    Serial.printf("Session %u: storage queue high-water %u, total dropped %u\n",
                  recordingQueue.sessionId(), recordingQueue.maxDepth(), recordingQueue.droppedSamples());
  }

}
