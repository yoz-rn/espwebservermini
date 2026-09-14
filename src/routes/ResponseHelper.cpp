#include <Arduino.h>
#include <ArduinoJson.h>

#include "ResponseHelper.h"

String buildStatusJson(bool success, const String& message) {
    JsonDocument doc;
    doc["success"] = success;
    doc["message"] = message;

    String output;
    serializeJson(doc, output);
    return output;
}