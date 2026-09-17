#pragma once

#include <Arduino.h>

#define APP_JSON "application/json"

String buildStatusJson(bool success, const String& message);
String getMimeType(const String& path);

