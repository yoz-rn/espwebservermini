#include <ArduinoJson.h>

#include "TaskManager.h"

#if !CONFIG_FREERTOS_USE_TRACE_FACILITY
#error "ACTIVATE CONFIG_FREERTOS_USE_TRACE_FACILITY IN sdkconfig.defaults"
#endif
#if !CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS
#error "ACTIVATE CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS IN sdkconfig.defaults"
#endif

const char* TaskManager::stateName(eTaskState state) {
    switch (state) {
        case eRunning: return "Running";
        case eReady: return "Ready";
        case eBlocked: return "Blocked";
        case eSuspended: return "Suspended";
        case eDeleted: return "Deleted";
        default: return "Invalid";
    }
}

bool TaskManager::toJson(String& out) {
    if (uxTaskGetNumberOfTasks() > MAX_TASKS) return false;

    uint32_t total = 0;
    UBaseType_t count = uxTaskGetSystemState(_buf, MAX_TASKS, &total);
    if (count == 0) return false;

    JsonDocument doc;
    doc["success"] = true;
    doc["total"] = total;
    JsonArray tasks = doc["tasks"].to<JsonArray>();

    for (UBaseType_t i = 0; i < count; i++) {
        const TaskStatus_t& s = _buf[i];
        JsonObject t = tasks.add<JsonObject>();
        t["n"] = s.xTaskNumber;
        t["name"] = s.pcTaskName;
        t["state"] = stateName(s.eCurrentState);
        t["prio"] = s.uxCurrentPriority;
        t["free"] = s.usStackHighWaterMark;
        t["rt"] = s.ulRunTimeCounter;
        #if CONFIG_FREERTOS_VTASKLIST_INCLUDE_COREID
        t["core"] = (s.xCoreID == tskNO_AFFINITY) ? -1 : (int)s.xCoreID;
        #endif
    }
    out = "";
    serializeJson(doc, out);
    return true;
}