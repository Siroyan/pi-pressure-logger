#pragma once
struct RuntimeStartup {
  bool acquisition=false, network=false;
  const char* error="Starting";
};

template<class QueueInit, class SamplingStart, class NetworkStart>
RuntimeStartup startRuntime(bool adc, bool mqtt, QueueInit initQueue,
                            SamplingStart startSampling, NetworkStart startNetwork) {
  RuntimeStartup result;
  bool queue=initQueue();
  result.acquisition=queue && adc && startSampling();
  result.network=mqtt && startNetwork();
  result.error=!queue ? "Sample queue unavailable" : !adc ? "ADC unavailable" :
      !result.acquisition ? "Sampling task unavailable" : !mqtt ? "MQTT resources unavailable" :
      !result.network ? "Network task unavailable" : nullptr;
  return result;
}
