/* PLEASE AWARE OF THE API STRING
 * BECAUSE IT IS HIERARCHICAL
 * DECREASED ORDER ALL THE WAY TO THE ROOT "/api/files"
 * ORDER MATTERS
 */

#include "FileRoutes.h"

namespace {
    bool isPathSafe(const String& path) {
        if (path.length() == 0) return false;
        if (path.indexOf("..") != -1) return false;
        return true;
    }
}

void registerFileRoutes(AsyncWebServer& server, FileManager& fileManager) {
    
    // Server API to Read File
    server.on("/api/files/view", HTTP_GET, [&fileManager](AsyncWebServerRequest *request) {
        if (!request->hasParam("path")) {
            request->send(400, APP_JSON, buildStatusJson(false, "parameter path required"));
            return;
        }
    
        String requestedPath = request->getParam("path")
                                      ->value();

        if (!isPathSafe(requestedPath)) {
            request->send(400, APP_JSON, buildStatusJson(false, "invalid path"));
            return;
        }

        String actualPath;
        bool isGzipped;
    
        if (!fileManager.resolveViewPath(requestedPath, actualPath, isGzipped)) {
            request->send(404, APP_JSON, buildStatusJson(false, "File not found: " + requestedPath));
            return;
        }
    
        String mimeType = getMimeType(requestedPath);
    
        AsyncWebServerResponse* response = request->beginResponse(LittleFS, actualPath, mimeType);
        
        if (isGzipped) response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    
    
    });

    // Server API to RENAME File or Directory
    server.on("/api/files/rename", HTTP_PATCH, [&fileManager](AsyncWebServerRequest *request) {
        if (!request->hasParam("path") || !request->hasParam("newPath")) {
            request->send(400, APP_JSON, buildStatusJson(false, "parameter path and newPath required"));
            return;
        }

        String oldPath = request->getParam("path")->value();
        String newPath = request->getParam("newPath")->value();

        if (!isPathSafe(oldPath) || !isPathSafe(newPath)) {
            request->send(400, APP_JSON, buildStatusJson(false, "Invalid path"));
            return;
        }

        if (fileManager.isProtectedPath(oldPath)) {
            request->send(403, APP_JSON, buildStatusJson(false, "Protected: cannot rename/move file inside /webapp: " + oldPath));
            return;
        }

        if (fileManager.isProtectedPath(newPath)) {
            request->send(403, APP_JSON, buildStatusJson(false, "Protected: cannot move/rename file into /webapp: " + newPath));
            return;
        }

        if (fileManager.fileExists(newPath) || fileManager.isDirectory(newPath)) {
            request->send(400, APP_JSON, buildStatusJson(false, "Target already exists: " + newPath));
            return;
        }

        bool ok = fileManager.renamePath(oldPath, newPath);
        request->send(ok ? 200 : 500, APP_JSON, buildStatusJson(ok, ok ? "Renamed to: " + newPath
                                                                       : "Failed to rename"));
    });

    // Server API to CREATE directory
    server.on("/api/files/mkdir", HTTP_POST, [&fileManager](AsyncWebServerRequest *request) {
        if (!request->hasParam("path")) {
            request->send(400, APP_JSON, buildStatusJson(false, "parameter path required"));
            return;
        }

        String path = request->getParam("path")->value();

        if (!isPathSafe(path)) {
            request->send(400, APP_JSON, buildStatusJson(false, "invalid path"));
            return;
        }

        if (fileManager.isProtectedPath(path)) {
            request->send(403, APP_JSON, buildStatusJson(false, "Protected: cannot create directory inside /webapp: " + path));
            return;
        }

        if (fileManager.fileExists(path) || fileManager.isDirectory(path)) {
            request->send(409, APP_JSON, buildStatusJson(false, "Already exists: " + path));
            return;
        }

        bool ok = fileManager.makeDirectory(path);
        request->send(ok ? 200 : 500, APP_JSON, buildStatusJson(ok, ok ? "Directory created: " + path
                                                                       : "Failed to create directory"));

    });
    
    // Server API to CREATE UPDATE File
    const size_t MAX_UPLOAD_SIZE = 25 * 1024; // 25 KB, sesuai kesepakatan

    // FileRoutes.cpp — ganti seluruh blok POST /api/files
    server.on("/api/files", HTTP_POST,
        [](AsyncWebServerRequest *request) {
            // Cuma jaring pengaman: kalau body kosong sama sekali, onBody gak akan pernah kepanggil
            if (request->contentLength() == 0) {
                request->send(400, APP_JSON, buildStatusJson(false, "No data file"));
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
                    request->send(400, APP_JSON, buildStatusJson(false, "Parameter path required"));
                    return;
                }
                String path = request->getParam("path")->value();

                if (!isPathSafe(path)) {
                    request->send(400, APP_JSON, buildStatusJson(false, "invalid path"));
                    return;
                }

                // /webapp is a protected, isolated static-asset tree: new files
                // and duplicates cannot be created there (only overwrite of an
                // existing file, i.e. an "update", is allowed). New/expanded
                // assets must go through a filesystem image reflash instead.
                if (fileManager.isProtectedPath(path)) {
                    String onConflictCheck = request->hasParam("onConflict") ? request->getParam("onConflict")->value() : "";

                    if (!fileManager.fileExists(path)) {
                        request->send(403, APP_JSON, buildStatusJson(false, "Protected: cannot create new file inside /webapp: " + path));
                        return;
                    }

                    if (onConflictCheck == "duplicate") {
                        request->send(403, APP_JSON, buildStatusJson(false, "Protected: cannot duplicate file inside /webapp: " + path));
                        return;
                    }
                    // existing file + no onConflict, or onConflict=overwrite:
                    // falls through to the normal conflict/overwrite flow below.
                }

                if (total > MAX_UPLOAD_SIZE) {
                    request->send(413, APP_JSON, buildStatusJson(false, "File exceed limit (25 KB)"));
                    return;
                }

                // ... kode sebelumnya ...

                // Gunakan nama variabel yang spesifik, misalnya fsTotal dan fsUsed
                size_t fsTotal, fsUsed;
                fileManager.getStorageBytes(fsTotal, fsUsed);
                size_t freeSpace = fsTotal - fsUsed;
                size_t margin = fsTotal * 0.15;
                size_t usableSpace = (freeSpace > margin) ? (freeSpace - margin) : 0;

                // Sekarang variabel 'total' merujuk dengan benar ke ukuran file yang sedang diunggah
                if (total > usableSpace) {
                    request->send(413, APP_JSON, buildStatusJson(false, "Insufficient space storage"));
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
                            request->send(500, APP_JSON, buildStatusJson(false, "Failed to duplicate"));
                            return;
                        }
                    } else {
                        String output;
                        JsonDocument doc;
                        doc["success"] = false;
                        doc["conflict"] = true;
                        doc["message"] = "File sudah ada";
                        serializeJson(doc, output);
                        request->send(409, APP_JSON, output);
                        return;
                    }
                }

                if (!fileManager.beginWrite(finalPath)) {
                    request->send(500, APP_JSON, buildStatusJson(false, "Failed to open and to write file"));
                    return;
                }
            }

            if (!fileManager.isWriteReady()) return; // sudah ditolak di index==0, abaikan chunk sisanya

            fileManager.writeChunk(data, len);

            if (index + len == total) {
                String writtenPath = fileManager.getWritePath();
                fileManager.endWrite();
                request->send(200, APP_JSON, buildStatusJson(true, "Upload success: " + writtenPath));
            }
        });

    // Server API to GET Storage Info
    server.on("/api/storage", HTTP_GET, [&fileManager](AsyncWebServerRequest *request) {
        JsonDocument doc;
        JsonObject obj = doc.to<JsonObject>();

        if (!fileManager.getStorageInfo(obj)) {
            request->send(500, APP_JSON, buildStatusJson(false, "Failed to read storage info"));
            return;
        }

        String output;
        serializeJson(doc, output);
        request->send(200, APP_JSON, output);
    });

    // Server API to DELETE File/Directory
    server.on("/api/files", HTTP_DELETE, [&fileManager](AsyncWebServerRequest *request) {
        if (!request->hasParam("path")) {
            request->send(400, APP_JSON, buildStatusJson(false, "parameter path required"));
            return;
        }
        String path = request->getParam("path")->value();

        if (!isPathSafe(path)) {
            request->send(400, APP_JSON, buildStatusJson(false, "invalid path"));
            return;
        }

        if (fileManager.isProtectedPath(path)) {
            request->send(403, APP_JSON, buildStatusJson(false, "Protected: cannot delete file/folder inside /webapp: " + path));
            return;
        }

        if (!fileManager.fileExists(path) && !fileManager.isDirectory(path)) {
            request->send(404, APP_JSON, buildStatusJson(false, "Path not found: " + path));
            return;
        }

        if (fileManager.isDirectory(path)) {
            if (!fileManager.isDirectoryEmpty(path)) {
                request->send(409, APP_JSON, buildStatusJson(false, "Directory not empty, delete its contents first: " + path));
                return;
            }
            bool ok = fileManager.deleteDirectory(path);
            request->send(ok ? 200 : 500, APP_JSON, buildStatusJson(ok, ok ? "Directory deleted: " + path
                                                                                     : "Failed to delete directory (filesystem error): " + path));
            return;
        }

        bool ok = fileManager.deleteFile(path);
        request->send(ok ? 200 : 500, APP_JSON, buildStatusJson(ok, ok ? "File deleted: " + path
                                                                       : "Failed to delete file (filesystem error): " + path));

    });

    // Server API to LIST File
    server.on("/api/files", HTTP_GET, [&fileManager](AsyncWebServerRequest *request) {
        String path = request->hasParam("path") 
        ? request->getParam("path")->value() 
        : "/";

        if (!isPathSafe(path)) {
            request->send(400, APP_JSON, buildStatusJson(false, "invalid path"));
            return;
        }

        JsonDocument doc;
        JsonArray files = doc.to<JsonArray>();

        bool ok = fileManager.listDirectory(path, files);

        if (!ok) {
            request->send(404, APP_JSON, buildStatusJson(false, "Direktori tidak ditemukan: " + path));
            return;
        }

        String output;
        serializeJson(doc, output);
        request->send(200, APP_JSON, output);
    });

}