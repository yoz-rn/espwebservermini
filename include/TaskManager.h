#pragma once

#include <Arduino.h>
#include <vector>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <ArduinoJson.h>

// #define configUSE_TRACE_FACILITY 1

struct TaskInfo {
    String name;
    UBaseType_t priority;
    eTaskState state;
    uint32_t stackHighWaterMark;

};

class TaskManager{
    
    public:
    std::vector<TaskInfo> getAllTasks();
    String toJson();
    
    private:
    static const UBaseType_t MAX_TASKS = 32;
    TaskStatus_t _taskStatusBuffer[MAX_TASKS];
    String stateToString(eTaskState state);
    
    
};