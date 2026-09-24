#include "FileManager.h"

namespace {
    // Root of the isolated static web asset tree (mounted from data/webapp/
    // at build time). Anything inside this prefix is considered protected.
    const char* PROTECTED_PREFIX = "/webapp";
}

bool FileManager::isProtectedPath(PATH) {
    String prefix = String(PROTECTED_PREFIX) + "/";
    return path == PROTECTED_PREFIX || path.startsWith(prefix);
}

bool FileManager::listDirectory(PATH, JsonArray& out) {
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

bool FileManager::fileExists(PATH) {
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

String FileManager::generateDuplicateName(PATH) {
    int lastSlash = path.lastIndexOf('/');
    int lastDot = path.lastIndexOf('.');

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

bool FileManager::beginWrite(PATH) {
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

bool FileManager::isDirectory(PATH) {
    File file = LittleFS.open(path);
    bool result =   file && file.isDirectory();
    file.close();
    return result;
}

bool FileManager::isDirectoryEmpty(PATH) {
    File dir = LittleFS.open(path);
    if (!dir || !dir.isDirectory()) return false;

    File entry = dir.openNextFile();
    bool empty = !entry;

    if (entry) entry.close();
    dir.close();
    return empty;
}

bool FileManager::deleteFile(PATH) {
    return LittleFS.remove(path);
}

bool FileManager::deleteDirectory(PATH) {
    return LittleFS.rmdir(path);
}

bool FileManager::isWriteReady() {
    return _writeReady;
}

String FileManager::getWritePath() {
    return _writePath;
}

bool FileManager::renamePath(const String& oldPath, const String& newPath) {
    return LittleFS.rename(oldPath, newPath);
}

bool FileManager::makeDirectory(PATH) {
    return LittleFS.mkdir(path);
}

bool FileManager::getStorageBytes(size_t& total, size_t& used) {
    total = LittleFS.totalBytes();
    used  = LittleFS.usedBytes();
    return total != 0; // false kalau LittleFS belum ter-mount
}

bool FileManager::getStorageInfo(JsonObject& out) {
    size_t total, used;
    if (!getStorageBytes(total, used)) return false;

    out["total"] = total;
    out["used"]  = used;
    out["free"]  = total - used;

    return true;
}