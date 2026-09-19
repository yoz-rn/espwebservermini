#pragma once

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>

#define APP_JSON "application/json"

String buildStatusJson(bool success, const String& message);
String getMimeType(const String& path);

