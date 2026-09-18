#pragma once

#include <Arduino.h>
#include <ESPAsyncWebServer.h>

#define APP_JSON "application/json"

String buildStatusJson(bool success, const String& message);
String getMimeType(const String& path);

