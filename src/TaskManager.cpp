#include "TaskManager.h"

String TaskManager::stateToString(eTaskState state) {
    switch (state)
    {
    case eRunning: return "Running";
    case eReady: return "Ready";
    case eBlocked: return "Blocked";
    case eSuspended: return "Suspended";
    case eDeleted: return "Deleted";
    default: return "Invalid";
    }
}

std::vector<TaskInfo> TaskManager::getAllTasks() {
    // /* @brief 
    //  * pakai buffer statis (_taskStatusBuffer) yang sudah
    //  * disediakan sebagai member, dipakai ulang tiap panggilan — 
    //  * menghindari alokasi heap berulang tiap kali endpoint /api/tasks 
    //  * di-poll dari frontend.
    //  * 
    //  * Alternatif "murni dinamis" (Strategi A) kalau nanti mau dicoba:
    //  * panggil uxTaskGetNumberOfTasks() dulu untuk tahu jumlah task
    //  * saat ini, lalu alokasikan array/vector persis seukuran itu
    //  * (misal via std::vector<TaskStatus_t> yang di-resize()).
    //  * Trade-off: alokasi ulang tiap request, tapi ukuran array
    //  * selalu pas dengan jumlah task aktual.
    //  */

    // UBaseType_t taskCount = uxTaskGetSystemState(
    // _taskStatusBuffer,
    // MAX_TASKS,
    // NULL
    // );

    std::vector<TaskInfo> result;
    // result.reserve(taskCount);

    // for (UBaseType_t i = 0; i < taskCount; i++) {
    //     TaskInfo info;
    //     info.name = String(_taskStatusBuffer[i].pcTaskName);
    //     info.priority = _taskStatusBuffer[i].eCurrentState == eDeleted
    //     ? 0
    //     : _taskStatusBuffer[i].uxCurrentPriority;
    //     info.state = _taskStatusBuffer[i].eCurrentState;
    //     info.stackHighWaterMark = _taskStatusBuffer[i].usStackHighWaterMark;

    //     result.push_back(info);
    // }
    return result;
    
}

String TaskManager::toJson() {
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();

    std::vector<TaskInfo> tasks = getAllTasks();

    for (const TaskInfo& task: tasks) {
        JsonObject obj = arr.add<JsonObject>();
        obj["name"] = task.name;
        obj["priority"] = task.state;
        obj["state"] = task.state;
        obj["stackHighWaterMark"] = task.stackHighWaterMark;
    }

    String output;
    serializeJson(doc, output);
    
    return output;
}