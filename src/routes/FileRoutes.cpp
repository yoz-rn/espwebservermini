/* PLEASE AWARE OF THE API STRING
 * BECAUSE IT IS HIERARCHICAL
 * DECREASED ORDER ALL THE WAY TO THE ROOT "/api/files"
 * ORDER MATTERS
 */

#include "FileRoutes.h"
#include "ResponseHelper.h"

void registerFileRoutes(AsyncWebServer& server, FileManager& fileManager) {
    
    // Server API to Read File
    server.on("/api/files/view", HTTP_GET, [&fileManager](AsyncWebServerRequest *request) {
        if (!request->hasParam("path")) {
            request->send(400, "application/json", buildStatusJson(false, "parameter path required"));
            return;
        }
    
        String requestedPath = request->getParam("path")
                                      ->value();
        String actualPath;
        bool isGzipped;
    
        if (!fileManager.resolveViewPath(requestedPath, actualPath, isGzipped)) {
            request->send(404, "application/json", buildStatusJson(false, "File not found: " + requestedPath));
            return;
        }
    
        String mimeType = getMimeType(requestedPath);
    
        AsyncWebServerResponse* response = request->beginResponse(LittleFS, actualPath, mimeType);
        
        if (isGzipped) response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    
    
    });
    
    // Server API to CREATE UPDATE File
    // FileRoutes.cpp — tambahan di dalam registerFileRoutes()

const size_t MAX_UPLOAD_SIZE = 25 * 1024; // 25 KB, sesuai kesepakatan

// FileRoutes.cpp — ganti seluruh blok POST /api/files
server.on("/api/files", HTTP_POST,
    [](AsyncWebServerRequest *request) {
        // Cuma jaring pengaman: kalau body kosong sama sekali, onBody gak akan pernah kepanggil
        if (request->contentLength() == 0) {
            request->send(400, "application/json", buildStatusJson(false, "No data file"));
        }
        // Kalau ada body, JANGAN kirim apa pun di sini — semua ditangani onBody
    },
    nullptr,
    [&fileManager](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {

        if (index == 0) {

            Serial.printf("[Create] index=%u len=%u total=%u hasPath=%d hasOnConflict=%d\n",
            index, len, total,
            request->hasParam("path"),
            request->hasParam("onConflict"));
            // Validasi semua di sini, SEBELUM mulai nulis chunk apa pun
            if (!request->hasParam("path")) {
                request->send(400, "application/json", buildStatusJson(false, "Parameter path required"));
                return;
            }
            String path = request->getParam("path")->value();

            if (total > MAX_UPLOAD_SIZE) {
                request->send(413, "application/json", buildStatusJson(false, "File exceed limit (25 KB)"));
                return;
            }

            size_t freeSpace = LittleFS.totalBytes() - LittleFS.usedBytes();
            size_t margin = LittleFS.totalBytes() * 0.15;
            size_t usableSpace = (freeSpace > margin) ? (freeSpace - margin) : 0;

            if (total > usableSpace) {
                request->send(413, "application/json", buildStatusJson(false, "Insufficient space storage"));
                return;
            }

            String finalPath = path;

            if (fileManager.fileExists(path)) {
                String onConflict = request->hasParam("onConflict") ? request->getParam("onConflict")->value() : "";

                if (onConflict == "overwrite") {
                    finalPath = path;
                } else if (onConflict == "duplicate") {
                    finalPath = fileManager.generateDuplicateName(path);
                    if (finalPath == "") {
                        request->send(500, "application/json", buildStatusJson(false, "Failed to duplicate"));
                        return;
                    }
                } else {
                    String output;
                    JsonDocument doc;
                    doc["success"] = false;
                    doc["conflict"] = true;
                    doc["message"] = "File sudah ada";
                    serializeJson(doc, output);
                    request->send(409, "application/json", output);
                    return;
                }
            }

            if (!fileManager.beginWrite(finalPath)) {
                request->send(500, "application/json", buildStatusJson(false, "Failed to open and to write file"));
                return;
            }
        }

        if (!fileManager.isWriteReady()) return; // sudah ditolak di index==0, abaikan chunk sisanya

        fileManager.writeChunk(data, len);

        if (index + len == total) {
            String writtenPath = fileManager.getWritePath();
            fileManager.endWrite();
            request->send(200, "application/json", buildStatusJson(true, "Upload success: " + writtenPath));
        }
    }
);

    // Server API to LIST File
    server.on("/api/files", HTTP_GET, [&fileManager](AsyncWebServerRequest *request) {
        String path = request->hasParam("path") 
        ? request->getParam("path")->value() 
        : "/";

        JsonDocument doc;
        JsonArray files = doc.to<JsonArray>();

        bool ok = fileManager.listDirectory(path, files);

        if (!ok) {
            request->send(404, "application/json", buildStatusJson(false, "Direktori tidak ditemukan: " + path));
            return;
        }

        String output;
        serializeJson(doc, output);
        request->send(200, "application/json", output);
    });

}