#pragma once

#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

#define PATH const String& path

class FileManager {
    
    public:
        // Create and Update Element
        String generateDuplicateName(PATH);
        bool beginWrite(PATH);
        bool writeChunk(uint8_t* data, size_t len);
        void endWrite();
        bool isWriteReady();
        String getWritePath();

        // Read Element
        bool listDirectory(PATH, JsonArray& out);
        bool fileExists(PATH);
        bool resolveViewPath(const String& requestedPath, String& actualPath, bool& isGzipped);

        // Delete Element
        bool isDirectory(PATH);
        bool isDirectoryEmpty(PATH);
        bool deleteFile(PATH);
        bool deleteDirectory(PATH);

        // Restructure Element
        bool renamePath(const String& oldPath, const String& newPath);
        bool makeDirectory(PATH);

    private:
        File _writeFile;
        String _writePath;
        bool _writeReady = false;

};