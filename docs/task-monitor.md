# Task Monitor: technical reference

A small web page plus one JSON endpoint that turns FreeRTOS's own task statistics into a `btop`-style view: every task, per-core CPU load, stack headroom and heap usage.

This page is for people who want to understand it, or reuse it in their own FreeRTOS project. If you just want to run the project, see the main [README](../README.md).

## Requirements

Enable these in `sdkconfig.defaults` (already on in this repo):

```
CONFIG_FREERTOS_USE_TRACE_FACILITY=y
CONFIG_FREERTOS_USE_STATS_FORMATTING_FUNCTIONS=y
CONFIG_FREERTOS_VTASKLIST_INCLUDE_COREID=y
CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS=y
```

These options can't be changed in the precompiled Arduino libraries, which is why the project uses the hybrid Arduino + ESP-IDF build.

* `USE_TRACE_FACILITY` and `GENERATE_RUN_TIME_STATS` are checked at compile time: the build stops with an `#error` if they're off.
* `VTASKLIST_INCLUDE_COREID` is only needed for the `core` column. If it's off, the `core` field is simply missing from the JSON.

## Files

| Part | Files |
|---|---|
| Backend | `src/TaskManager.h`, `src/TaskManager.cpp`, `src/routes/TaskRoutes.h`, `src/routes/TaskRoutes.cpp` |
| Frontend | `data/task-monitor.html`, `data/assets/task-monitor/task-monitor.js`, `data/assets/task-monitor/task-monitor.css` |
| Shared look | `data/assets/core/` (theme, base style, sidebar) |

## API: `GET /api/tasks`

```json
{
  "success": true,
  "total": 71825678,
  "heap": { "free": 237736, "min": 232272, "blk": 110592, "size": 331108 },
  "tasks": [
    { "n": 15, "name": "async_tcp", "state": "Running", "prio": 10, "free": 13712, "rt": 3785, "core": -1 }
  ]
}
```

| Field | Meaning |
|---|---|
| `total` | Total run-time counter (32-bit, wraps around) |
| `heap.free` | Free internal heap, in bytes |
| `heap.min` | Lowest `free` since boot. It only ever goes down |
| `heap.blk` | Largest single free block (the biggest allocation that can succeed) |
| `heap.size` | Total internal heap (free + allocated) |
| `n` | Task number, unique per task. Use it to match tasks between polls |
| `name`, `prio` | Task name and current priority |
| `state` | `Running`, `Ready`, `Blocked`, `Suspended`, `Deleted` |
| `free` | Stack headroom: the least free stack the task has ever had, in bytes |
| `rt` | Cumulative run time of this task (same unit as `total`) |
| `core` | `0` or `1`; `-1` means the task isn't pinned to a core |

If there are more tasks than the backend buffer holds (`MAX_TASKS`, 48 by default), the endpoint answers **HTTP 500** and the page shows `error: HTTP 500`. Raise `MAX_TASKS` in `TaskManager.h` if you need more (it's a static buffer, so it costs RAM).

## How the numbers are computed

**CPU % per task.** Poll twice and compare:

```
pct = 100 * (rt_now - rt_before) / (total_now - total_before)
```

* Subtract as **unsigned 32-bit** numbers so the counter wrapping around doesn't matter.
* Match tasks by `n`, not by their position in the array.
* The first sample has nothing to compare against, so it shows `--`.
* After a board reboot the counters restart from zero, so the first sample after a reboot isn't meaningful and should be dropped.

**Core load.** Each core has an idle task (`IDLE0`, `IDLE1`). Core load = `100 - idle%`. On a dual-core chip the per-task percentages add up to about 200%, because each core has its own 100%.

**Memory bars.** `MEM` = `size - free` (used now). `PEAK` = `size - min` (highest use since boot). Two hints when reading them:

* `blk` being smaller than `free` is normal: internal RAM comes in separate regions. What matters is the *trend*. A `blk` that keeps shrinking over days points at fragmentation.
* A memory leak shows up as `MEM` climbing steadily while idle. A `PEAK` that jumps and then stays put is just a past spike.

## Good to know

* A task that waits forever for an event can be reported as **Suspended** (`esp_timer` in the sample). That doesn't necessarily mean it's stuck.
* `toJson()` uses one shared buffer, so call it from a single task only (the web server's task).
* `printTasks()` and `printCpuLoad()` print the same information to the Serial Monitor. `printCpuLoad()` blocks for about one second, so call it from `setup()`, never from a request handler.
* Cost: with the page open and a 1 second refresh, a rough measurement showed about 1.7 percentage points of extra load on core 0, mostly networking rather than reading task statistics. Polling pauses automatically when the browser tab is hidden, and the refresh interval can be changed on the page.