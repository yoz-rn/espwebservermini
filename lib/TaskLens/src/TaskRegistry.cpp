#include <cstring>
#include "TaskRegistry.h"

namespace {
    struct Entry { TaskHandle_t h; char tag[TaskRegistry::TAG_LEN]; };
    Entry entries[TaskRegistry::MAX_USER_TASKS] = {};
    portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
    
}

namespace {
    struct TrampolineArgs { 
        TaskFunction_t fn; 
        void* param; 
        char tag[TaskRegistry::TAG_LEN]; 
    };
    
    TrampolineArgs* makeArgs(TaskFunction_t fn, void* param, const char* tag) {
        auto* args = static_cast<TrampolineArgs*>(pvPortMalloc(sizeof(TrampolineArgs)));
        if (!args) return nullptr;
        args->fn = fn;
        args->param = param;
        strlcpy(args->tag, tag ? tag : "", TaskRegistry::TAG_LEN);
        return args;
    }

    void trampoline(void* raw) {
        auto* args = static_cast<TrampolineArgs*>(raw);
        TaskFunction_t fn = args->fn;
        void* param = args->param;

        TaskRegistry::add(xTaskGetCurrentTaskHandle(), args->tag);
        vPortFree(args);

        fn(param);

        TaskRegistry::remove(xTaskGetCurrentTaskHandle());
        vTaskDelete(NULL);
    }
}



bool TaskRegistry::add(TaskHandle_t h, const char* tag) {
    bool ok = false;
    portENTER_CRITICAL(&mux);
    for (auto& e : entries)  {
        if (e.h == nullptr) {
            e.h = h;
            strlcpy(e.tag, tag ? tag : "", TAG_LEN);
            ok = true;
            break;
        }
    }
    portEXIT_CRITICAL(&mux);
    return ok;
}

void TaskRegistry::remove(TaskHandle_t h) {
    portENTER_CRITICAL(&mux);
    for (auto& e : entries)
        if (e.h == h) {
            e.h = nullptr;
            e.tag[0] = '\0';
        }
    portEXIT_CRITICAL(&mux);
}

bool TaskRegistry::lookup(TaskHandle_t h, char* out, size_t outLen) {
    bool found = false;
    portENTER_CRITICAL(&mux);
    for (auto& e : entries) {
        if (e.h == h) {
            strlcpy(out, e.tag, outLen);
            found = true;
            break;
        }
    }
    portEXIT_CRITICAL(&mux);
    return found;
}

void TaskRegistry::reconcile(const TaskStatus_t* liveTasks, UBaseType_t liveCount) {
    portENTER_CRITICAL(&mux);
    for (auto& e : entries) {
        if (e.h == nullptr) continue;
        bool alive = false;
        for (UBaseType_t i = 0; i < liveCount; i++) {
            if (liveTasks[i].xHandle == e.h) {
                alive = true;
                break;
            }
            if (!alive) {
                e.h = nullptr;
                e.tag[0] = '\0';
            }
        }
    }
    portEXIT_CRITICAL(&mux);
}

BaseType_t TaskRegistry::monitoredTaskCreate(
    TaskFunction_t userFunc, const char* name, uint32_t stackDepth,
    void* param, UBaseType_t priority, TaskHandle_t* handle,
    const char* tag,
    BaseType_t coreID)
{
    TrampolineArgs* args = makeArgs(userFunc, param, tag);
    if (!args) return errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY;

    BaseType_t ok = xTaskCreatePinnedToCore(
        trampoline, name, stackDepth, args, priority, handle, coreID
    );

    if (ok != pdPASS) {
        vPortFree(args);
    }
    return ok;
}