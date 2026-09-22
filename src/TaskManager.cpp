#include <ArduinoJson.h>
#include <esp_heap_caps.h>

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
    TaskRegistry::reconcile(_buf, count);

    multi_heap_info_t heap;
    heap_caps_get_info(&heap, MALLOC_CAP_INTERNAL);

    JsonDocument doc;
    doc["success"] = true;
    doc["total"] = total;
    doc["up"] = (uint32_t)(esp_timer_get_time() / 1000000);   // detik sejak boot

    JsonObject h = doc["heap"].to<JsonObject>();
    h["free"] = heap.total_free_bytes;
    h["min"] = heap.minimum_free_bytes;
    h["blk"] = heap.largest_free_block;
    h["size"] = heap.total_free_bytes + heap.total_allocated_bytes;

    JsonArray tasks = doc["tasks"].to<JsonArray>();

    for (UBaseType_t i = 0; i < count; i++) {
        const TaskStatus_t& s = _buf[i];
        char tag[TaskRegistry::TAG_LEN];
        bool isUser = TaskRegistry::lookup(s.xHandle, tag, sizeof(tag));
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
        t["user"] = isUser;
        if (isUser) t["tag"] = tag;
    }
    out = "";
    serializeJson(doc, out);
    return true;
}

namespace {
    struct Snap { TaskStatus_t* t; UBaseType_t n; uint32_t total; };

    bool takeSnap(Snap& s) {
        s.n = uxTaskGetNumberOfTasks();
        s.t = (TaskStatus_t*)pvPortMalloc(s.n * sizeof(TaskStatus_t));
        if (!s.t) return false;
        s.n = uxTaskGetSystemState(s.t, s.n, &s.total);
        return true;
    }
}

void TaskManager::printTasks() {
    UBaseType_t n = uxTaskGetNumberOfTasks();
    TaskStatus_t* arr = (TaskStatus_t*)pvPortMalloc(n * sizeof(TaskStatus_t));
    if (!arr) return;

    uint32_t total = 0;
    n = uxTaskGetSystemState(arr, n, &total);

    for (UBaseType_t i = 0; i < n; i++) {
        const TaskStatus_t& s = arr[i];
        Serial.printf("%-16s state=%s prio=%u stackFree=%u",
                      s.pcTaskName, stateName(s.eCurrentState),
                      (unsigned)s.uxCurrentPriority, (unsigned)s.usStackHighWaterMark);
        #if CONFIG_FREERTOS_VTASKLIST_INCLUDE_COREID
        Serial.printf(" core=%d", (s.xCoreID == tskNO_AFFINITY) ? -1 : (int)s.xCoreID);
        #endif
        Serial.println();
    }
    vPortFree(arr);
}

void TaskManager::printCpuLoad() {
    Snap a, b;
    if (!takeSnap(a)) return;
    delay(1000);                                   // jeda pengukuran
    if (!takeSnap(b)) { vPortFree(a.t); return; }

    uint32_t dTotal = b.total - a.total;           // unsigned: aman terhadap wrap-around
    if (dTotal == 0) {
        Serial.println("[TaskManager] dTotal = 0, tidak ada selisih waktu untuk dibandingkan.");
    } else {
        float sum = 0;
        for (UBaseType_t i = 0; i < b.n; i++) {
            for (UBaseType_t j = 0; j < a.n; j++) {    // a.n, bukan b.n
                if (a.t[j].xTaskNumber == b.t[i].xTaskNumber) {
                    uint32_t dTask = b.t[i].ulRunTimeCounter - a.t[j].ulRunTimeCounter;
                    float pct = 100.0f * dTask / dTotal;
                    sum += pct;
                    Serial.printf("%-16s %6.2f%%\n", b.t[i].pcTaskName, pct);
                    break;
                }
            }
        }
        // Di ESP32 dual-core, SUM mendekati 200%: tiap core punya 100% waktunya sendiri.
        Serial.printf("SUM: %.2f%% (dTotal=%lu)\n", sum, (unsigned long)dTotal);
    }
    vPortFree(a.t);
    vPortFree(b.t);
}