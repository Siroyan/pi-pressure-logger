#pragma once
// Compile-only placeholders. Never use this build to connect to AWS.
const char* ssid = "test-only";
const char* password = "test-only";
const char* aws_iot_endpoint = "example.invalid";
const int aws_iot_port = 8883;
const char* thing_name = "PressureLoggerTest";
const char* aws_iot_topic = "pressure_logger/data";
const char* aws_root_ca = "";
const char* device_cert = "";
const char* device_key = "";
