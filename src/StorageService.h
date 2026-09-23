#pragma once
#include "RecordingQueue.h"
#include "RecordingWriter.h"
#include <freertos/semphr.h>

// SD is owned by this worker after setup. UI requests never perform disk I/O.
class StorageService {
  struct Request { bool remove; char name[96]; };
  SDManager& sd;
  RecordingQueue& samples;
  RecordingWriter writer;
  QueueHandle_t requests = nullptr;
  SemaphoreHandle_t resultMutex = nullptr;
  SemaphoreHandle_t busMutex = nullptr;
  std::vector<String> resultFiles;
  std::vector<long> resultSizes;
  bool resultReady = false, resultSuccess = true;
public:
  StorageService(SDManager& storage, RecordingQueue& queue) : sd(storage), samples(queue), writer(storage) {}
  ~StorageService() {
    if (requests) vQueueDelete(requests);
    if (resultMutex) vSemaphoreDelete(resultMutex);
    if (busMutex) vSemaphoreDelete(busMutex);
  }
  bool init() {
    requests = xQueueCreate(1, sizeof(Request));
    resultMutex = xSemaphoreCreateMutex();
    busMutex = xSemaphoreCreateMutex();
    return ready();
  }
  bool ready() const { return requests && resultMutex && busMutex; }
  bool tryBeginDisplay() { return ready() && xSemaphoreTake(busMutex,0)==pdTRUE; }
  void endDisplay() { xSemaphoreGive(busMutex); }
  bool requestList() {
    Request request{};
    return ready() && xQueueSend(requests,&request,0)==pdTRUE;
  }
  bool requestDelete(const String& name) {
    Request request{};
    request.remove=true;
    if (name.length() >= sizeof(request.name)) return false;
    memcpy(request.name,name.c_str(),name.length()+1);
    return ready() && xQueueSend(requests,&request,0)==pdTRUE;
  }
  bool takeResult(std::vector<String>& files, std::vector<long>& sizes, bool& success) {
    if (!ready() || xSemaphoreTake(resultMutex,0)!=pdTRUE) return false;
    bool ready=resultReady;
    if (ready) {
      files.swap(resultFiles); sizes.swap(resultSizes);
      success=resultSuccess; resultReady=false;
    }
    xSemaphoreGive(resultMutex);
    return ready;
  }
  void step() {
    if (!ready()) return;
    xSemaphoreTake(busMutex,portMAX_DELAY);
    RecordEvent event;
    // Bound worker iterations too: allow the idle task to run under sustained load.
    for (unsigned i=0; i<32 && samples.receive(event); ++i) writer.process(event);
    sd.poll();
    Request request;
    if (samples.empty() && !sd.isRecording() && requests && xQueueReceive(requests,&request,0)==pdTRUE) {
      bool success=!request.remove || sd.deleteFile(String(request.name));
      auto files=sd.getLogFileList();
      std::vector<long> sizes;
      for (const auto& name:files) sizes.push_back(sd.getFileSize(name));
      xSemaphoreTake(resultMutex,portMAX_DELAY);
      resultFiles.swap(files); resultSizes.swap(sizes);
      resultSuccess=success; resultReady=true;
      xSemaphoreGive(resultMutex);
    }
    xSemaphoreGive(busMutex);
  }
};
