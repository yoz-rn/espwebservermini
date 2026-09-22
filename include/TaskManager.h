#pragma once

#include <Arduino.h>
#include <TaskRegistry.h>

class TaskManager {
    public:
    bool toJson(String& out);

    void printTasks();
    void printCpuLoad();

    private:
    static const UBaseType_t MAX_TASKS = 48;
    TaskStatus_t _buf[MAX_TASKS];
    static const char* stateName(eTaskState state);
};