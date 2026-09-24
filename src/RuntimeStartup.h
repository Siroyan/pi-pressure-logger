#pragma once
struct RuntimeStartup {
  bool acquisition=false, network=false, storage=false;
  const char* error="Starting";
};

// Storage workers must be ready before acquisition can produce recorded data.
template<class QueueInit, class StorageInit, class StorageStart, class SamplingStart, class NetworkStart>
RuntimeStartup startRuntime(bool adc, bool mqtt, QueueInit initQueue,
    StorageInit initStorage, StorageStart startStorage, SamplingStart startSampling, NetworkStart startNetwork) {
  RuntimeStartup result;
  bool queue=initQueue();
  bool storageResources=queue && initStorage();
  result.storage=storageResources && startStorage();
  result.acquisition=result.storage && adc && startSampling();
  result.network=mqtt && startNetwork();
  result.error=!queue ? "Sample queues unavailable" : !storageResources ? "Storage resources unavailable" :
      !result.storage ? "Storage task unavailable" : !adc ? "ADC unavailable" :
      !result.acquisition ? "Sampling task unavailable" : !mqtt ? "MQTT resources unavailable" :
      !result.network ? "Network task unavailable" : nullptr;
  return result;
}
