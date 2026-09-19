#include <Arduino.h>
#include <ArduinoJson.h>

#include "ResponseHelper.h"

struct MimeMapping
{
    const char* ext;
    const char* type;
};

static const MimeMapping mimeTable[] = {
    { ".html", "text/html" },
    { ".css",  "text/css" },
    { ".js",   "application/javascript" },
    { ".txt",  "text/plain" },
    { ".json", "application/json" },
    { ".png",  "image/png" },
    { ".jpg",  "image/jpeg" },
    { ".jpeg", "image/jpeg" },
    { ".svg",  "image/svg+xml" },
    { ".webp", "image/webp" },
    { ".avif", "image/avif" },
    { ".wasm", "application/wasm"},
    { ".mid",  "audio/midi"}
};


String buildStatusJson(bool success, const String& message) {
    JsonDocument doc;
    doc["success"] = success;
    doc["message"] = message;

    String output;
    serializeJson(doc, output);
    return output;
}

String getMimeType(const String& path) {
    for (const auto& entry : mimeTable)
        if (path.endsWith(entry.ext))
            return entry.type;

    return "application/octet-stream";
}