#pragma once

#include <Arduino.h>

class TaskManager {
    public:
    bool toJson(String& out);

    private:
    static const UBaseType_t MAX_TASKS = 48;
    TaskStatus_t _buf[MAX_TASKS];
    static const char* stateName(eTaskState state);
};