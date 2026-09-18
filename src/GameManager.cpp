#include "GameManager.h"

void GameManager::begin() {
    _prefs.begin("games", false);
}

uint32_t GameManager::getHighScore(const String& game) {
    String key = game + "_hs";
    return _prefs.getUInt(key.c_str(), 0);
}

bool GameManager::SubmitScore(const String& game, uint32_t score, uint32_t& outHighScore) {
    String key = game + "_hs";
    uint32_t currentHighScore = _prefs.getUInt(key.c_str(), 0);

    if (score > currentHighScore) {
        if (_prefs.putUInt(key.c_str(), score) == 0) return false;
        currentHighScore = score;
    }
    outHighScore = currentHighScore;
    return true;
}