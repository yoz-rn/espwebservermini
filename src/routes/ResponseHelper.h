#pragma once

#include <Arduino.h>

String buildStatusJson(bool success, const String& message);
String getMimeType(const String& path);