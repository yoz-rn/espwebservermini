#pragma once

#include <Arduino.h>
#include <Preferences.h>


class GameManager {
    public:
        void begin();

        uint32_t getHighScore(const String& game);
        bool SubmitScore(const String& game, uint32_t score, uint32_t& outHighScore);

    private:
        Preferences _prefs;

};