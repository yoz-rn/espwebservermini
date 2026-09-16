#include "FileManager.h"

bool FileManager::listDirectory(const String& path, JsonArray& out) {
    File root = LittleFS.open(path);

    Serial.printf("[FileManager] path='%s' | root valid=%d | isDirectory=%d\n",
                  path.c_str(), (bool)root, root.isDirectory());

    if (!root || !root.isDirectory()) return false;

    File entry = root.openNextFile();

    while (entry) {
        String name = String(entry.name());
        String fullPath = path;
        if (!fullPath.endsWith("/")) fullPath += "/";
        fullPath += name;

        JsonObject obj = out.add<JsonObject>();
        obj["name"] = name;
        obj["path"] = fullPath;
        obj["isDir"] = entry.isDirectory();
        obj["size"] = entry.size();

        entry.close();
        entry = root.openNextFile();
    }

    root.close();
    return true;
    
}

bool FileManager::fileExists(const String& path) {
    File file = LittleFS.open(path);
    bool valid = file && !file.isDirectory();
    file.close();
    return valid;
}

bool FileManager::resolveViewPath(const String& requestedPath, String& actualPath, bool& isGzipped) {
    if (fileExists(requestedPath)) {
        actualPath = requestedPath;
        isGzipped = false;
        return true;
    }

    String gzPath = requestedPath + ".gz";
    if (fileExists(gzPath)) {
        actualPath = gzPath;
        isGzipped = true;
        return true;
    }

    return false;
}

String FileManager::generateDuplicateName(const String& path) {
    int lastSlash = path.lastIndexOf('/');
    int lastDot = path.indexOf('.');

    bool hasExtension = (lastDot > lastSlash);

    String dir = path.substring(0, lastSlash + 1);
    String base = hasExtension ? path.substring(lastSlash + 1, lastDot)
                               : path.substring(lastSlash + 1);
    String ext = hasExtension ? path.substring(lastDot)
                              : "";
    const int MAX_ATTEMPTS = 1000;
    for (int i = 1; i <= MAX_ATTEMPTS; i++) {
        String candidate = dir + base + "-" + String(i) + ext;
        if (!fileExists(candidate)) return candidate;
    }
    return "";
}

bool FileManager::beginWrite(const String& path) {
    _writeFile = LittleFS.open(path, "w");
    if (!_writeFile) {
        _writeReady = false;
        return false;
    }
    _writePath = path;
    _writeReady = true;
    return true;
}

bool FileManager::writeChunk(uint8_t* data, size_t len) {
    if (!_writeReady) return false;
    size_t written = _writeFile.write(data, len);
    return written == len;
}

void FileManager::endWrite() {
    _writeFile.close();
    _writeReady = false;
}

bool FileManager::isWriteReady() {
    return _writeReady;
}

String FileManager::getWritePath() {
    return _writePath;
}