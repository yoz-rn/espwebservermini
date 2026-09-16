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
    const size_t MAX_UPLOAD_SIZE = 25 * 1024;

    server.on("api/files", HTTP_POST,
        [&fileManager](AsyncWebServerRequest *request) {

            if (!request->hasParam("path")) {
                request->send(400, "application/json", buildStatusJson(false, "parameter path requierd"));
                return;
            }
            String path = request->getParam("path")
                                 ->value();

            size_t contentLength = request->contentLength();

            if (contentLength > MAX_UPLOAD_SIZE) {
                request->send(413, "application/json", buildStatusJson(false, "file exceed limit"));
                return;
            }
            
            size_t freeSpace = LittleFS.totalBytes() - LittleFS.usedBytes();
            size_t margin = LittleFS.totalBytes() * 0.15;
            size_t usableSpace = (freeSpace > margin) ? (freeSpace - margin)
                                                      : 0;
            if (contentLength > usableSpace) {
                request->send(413, "application/json", buildStatusJson(false, "insufficient space"));
                return;
            }

            String finalPath = path;
            if (fileManager.fileExists(path)) {
                String onConflict = request->hasParam("onConflict") ? request->getParam("onConflict")
                                                                             ->value()
                                                                    : "";
                if (onConflict == "overwrite") finalPath == path;
                else if (onConflict == "duplicate") {
                    finalPath = fileManager.generateDuplicateName(path);
                    if (finalPath == "") {
                        request->send(500, "application/json", buildStatusJson(false, "failed to duplicate"));
                        return;
                    }
                }
                else {
                    String output;
                    JsonDocument doc;
                    doc["success"] = false;
                    doc["conflict"] = true;
                    doc["message"] = "File exist";  
                    serializeJson(doc, output);
                    request->send(409, "application/json", output);
                    return;
                    }
                
            }
            if (!fileManager.beginWrite(finalPath)) {
                request->send(500, "application/json", buildStatusJson(false, "failed to open file (to be written)"));
                return;
            }

        },
        nullptr,
        [&fileManager](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            if (!fileManager.isWriteReady()) return;

            fileManager.writeChunk(data, len);

            if (index + len == total) {
                String writtenPath = fileManager.getWritePath();
                fileManager.endWrite();
                request->send(200, "application/json", buildStatusJson(true, "Upload success: " + writtenPath));
            }
        });

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