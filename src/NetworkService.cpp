#include "NetworkService.h"
#ifndef PRESSURE_OFFLINE
#ifdef PRESSURE_TEST_CONFIG
#include "../test/support/FirmwareConfig.h"
#else
#include "../secure/aws_certificates.h"
#include "../secure/config.h"
#endif

NetworkService::NetworkService(RecordingQueue& samples)
  : queue(samples), wifi(ssid,password),
    mqtt(&tls,aws_iot_endpoint,aws_iot_port,thing_name,aws_iot_topic,aws_root_ca,device_cert,device_key) {}
void NetworkService::init() { wifi.init(); time.init(); mqtt.init(); }
void NetworkService::step() {
  wifi.checkConnection(); time.poll();
  RecordEvent event;
  if (queue.receiveNetwork(event)) {
    if (event.kind==RecordKind::Sample && event.session)
      mqtt.offer({event.sample.p0,event.sample.p1,event.sample.timestamp,event.session,event.sequence});
    else mqtt.clearPending();
  }
  mqtt.loop(wifi.isConnected() && time.isTimeSynced());
}
bool NetworkService::ready() { return mqtt.isReady(); }
bool NetworkService::enabled() const { return true; }
bool NetworkService::wifiConnected() { return wifi.isConnected(); }
bool NetworkService::mqttConnected() { return mqtt.isConnected(); }
TimeManager* NetworkService::timeSource() { return &time; }
#else
NetworkService::NetworkService(RecordingQueue&) {}
void NetworkService::init() {}
void NetworkService::step() {}
bool NetworkService::ready() { return true; }
bool NetworkService::enabled() const { return false; }
bool NetworkService::wifiConnected() { return false; }
bool NetworkService::mqttConnected() { return false; }
TimeManager* NetworkService::timeSource() { return nullptr; }
#endif
