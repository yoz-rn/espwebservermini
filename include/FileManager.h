#pragma once

#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

class FileManager {
    
    public:
        // Create and Update Element
        String generateDuplicateName(const String& path);
        bool beginWrite(const String& path);
        bool writeChunk(uint8_t* data, size_t len);
        void endWrite();
        bool isWriteReady();
        String getWritePath();

        // Read Element
        bool listDirectory(const String& path, JsonArray& out);
        bool fileExists(const String& path);
        bool resolveViewPath(const String& requestedPath, String& actualPath, bool& isGzipped);

    private:
        File _writeFile;
        String _writePath;
        bool _writeReady = false;

};