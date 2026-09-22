#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

class TaskRegistry
{
private:
    
public:
    static constexpr size_t MAX_USER_TASKS = 16;
    static constexpr size_t TAG_LEN = 16;

    static bool add(TaskHandle_t h, const char* tag);
    static void remove(TaskHandle_t h);
    static bool lookup(TaskHandle_t h, char* out, size_t outLen);
    static void reconcile(const TaskStatus_t* liveTasks, UBaseType_t liveCount);

    static BaseType_t monitoredTaskCreate(
        TaskFunction_t userFunc, const char* name, uint32_t stackDepth,
        void* param, UBaseType_t priority, TaskHandle_t* handle,
        const char* tag = nullptr,
        BaseType_t coreID = tskNO_AFFINITY
    );    
};

