# Network Manager: technical reference

A dual-mode (AP + STA) WiFi manager for ESP32: captive-portal-style provisioning, a live connection status indicator, exponential-backoff auto-reconnect that's polite to anyone still connected to the AP, and a "forget this network" flow — all event-driven, no polling task.

This page is for people who want to understand it, or reuse it in their own ESP32 project. If you just want to run the project, see the main [README](../README.md).

## Requirements

No special `sdkconfig` flags needed (unlike [Task Monitor](TASK-MONITOR.md)) — this runs on stock Arduino-ESP32. Libraries used, all bundled with the core:

```
WiFi.h        // AP + STA modes, events
ESPmDNS.h     // http://esp32.local
Preferences.h // NVS storage for saved credentials
esp_timer.h   // one-shot software timers (backoff, deferred execution)
```

## Files

| Part | Files |
|---|---|
| Backend | `include/NetworkManager.h`, `src/NetworkManager.cpp`, `src/routes/WifiRoutes.h`, `src/routes/WifiRoutes.cpp` |
| Frontend | `data/wifi-setup.html`, `data/assets/wifi-setup/wifi-setup.css`, `data/assets/wifi-setup/wifi-setup.js` |
| Shared look | `data/assets/core/` (theme, base style, sidebar — the connection status dot lives in `sidebar.js`/`sidebar.css`) |

## API

All endpoints return JSON (`{"success": bool, "message": string}` shape, plus extra fields where noted).

| Endpoint | Method | Purpose |
|---|---|---|
| `/api/wifi-config` | POST | Start provisioning: test `ssid`/`password` (form-urlencoded body) in the background, save to NVS on success |
| `/api/wifi-status` | GET | Poll target for provisioning **and** live connection state (see below) |
| `/api/wifi-reconnect` | POST | Reconnect using the already-saved credentials, no re-entry needed |
| `/api/wifi-disconnect` | POST | Drop the STA link without erasing credentials (stays AP-only until reconnected) |
| `/api/wifi-forget` | POST | Disconnect (if connected) then erase the saved credentials |

`GET /api/wifi-status` response:

```json
{
  "status": "IDLE",
  "message": "",
  "staConnected": true,
  "staIP": "192.168.100.126",
  "savedSSID": "Happyfamily"
}
```

| Field | Meaning |
|---|---|
| `status`/`message` | Provisioning state machine: `IDLE`, `TESTING`, `SUCCESS`, `FAILED` — only meaningful right after a `POST /api/wifi-config` |
| `staConnected` | Live STA link state, independent of provisioning |
| `staIP` | STA IP if connected, `""` otherwise |
| `savedSSID` | The SSID currently stored in NVS, `""` if none — lets the UI show a saved-network entry even while AP-only |

`requestForget()`/`requestDisconnect()`/`requestReconnect()` (and their endpoints) always answer optimistically (`200`, "in progress") — see [Deferred execution](#deferred-execution-dont-touch-the-network-stack-from-a-request-handler) for why the real result has to be read back from `/api/wifi-status` a moment later, not from the response itself.

## How auto-reconnect works

**Exponential backoff, one-shot timer.** `esp_timer_start_once()` is re-armed manually on every fire (`scheduleReconnect()`), not `esp_timer`'s periodic mode — the interval changes every attempt, and periodic mode's interval is fixed at creation.

```
interval = min(interval * 2, RECONNECT_MAX_MS)   // after every real failed attempt
```

Defaults: `RECONNECT_BASE_MS = 15000`, `RECONNECT_MAX_MS = 300000`. Reset to base on `STA_GOT_IP` (success) and at the start of a new disconnect episode.

**Two things gate an attempt, beyond just "are we disconnected":**

* **Attempt cap** (`MAX_RECONNECT_ATTEMPTS`, default 5) — after this many real failures in one episode, the device stops trying until the next disconnect episode (or a manual reconnect/restart). Without a cap, a permanently-down home router means the device retries forever in the background.
* **AP-client hold** — ESP32's AP and STA share a single radio. Every `WiFi.begin()` call forces a channel realignment (CSA) that can drop clients currently connected to the AP. If `_apClientCount > 0` (tracked via `AP_STACONNECTED`/`AP_STADISCONNECTED` events) when the timer fires, the attempt is skipped and the timer is **rescheduled at the same interval** — `scheduleReconnect(/*advanceBackoff=*/false)` — so waiting doesn't burn into the attempt cap or the backoff curve.

**Boot-time gap to watch for:** a failed `beginSTA()` call in `setup()` does **not** by itself start the backoff cycle — `onWifiEvent()`'s disconnect handler only schedules a retry on the transition *from* connected, and at boot the device was never connected in the first place. Call `notifyInitialSTAFailure()` explicitly after a failed `beginSTA()` to cover this case; otherwise a device that fails to connect at boot silently stays AP-only forever, even with valid saved credentials.

**Manual actions need their own flag.** `WiFi.disconnect()` fires the same `STA_DISCONNECTED` event as an unplanned drop. Without a way to tell them apart, every manual "Disconnect" click gets auto-reconnected ~15 seconds later by the same logic that's supposed to handle *unplanned* drops. Fix: a one-shot flag (`_manualDisconnectRequested`, set before the deliberate `WiFi.disconnect()` call, consumed — read and reset — the next time the disconnect event fires) that skips scheduling just once.

## Deferred execution: don't touch the network stack from a request handler

This is the one gotcha worth internalizing beyond this specific feature.

`AsyncWebServer` route handlers run **inside the `AsyncTCP`/`lwip` thread itself** — the same thread that still has to send the HTTP response for the request currently being handled. Calling something disruptive to the network stack (`WiFi.disconnect()`, `WiFi.begin()`) or something blocking (`Preferences::remove()`, a flash write) *synchronously* inside that handler can corrupt that connection's TCP bookkeeping. On this project it surfaced as an `lwip` assert crash (`tcp_update_rcv_ann_wnd`) — sometimes instantly, sometimes only after a few repeated cycles, depending on timing.

The fix that generalizes: **never call disruptive network operations directly from a route handler.** Schedule them instead, reply immediately, and run the real work slightly later, off that thread:

```cpp
// in the handler
networkManager.requestDisconnect();          // just schedules
request->send(200, APP_JSON, buildStatusJson(true, "Memutuskan koneksi..."));

// NetworkManager: schedule + run later
void NetworkManager::requestDisconnect() { scheduleDeferred(DeferredOp::DISCONNECT); }

void NetworkManager::scheduleDeferred(DeferredOp op) {
    _pendingOp = op;
    // ... esp_timer_create() once, esp_timer_start_once(_deferredTimer, 300000) ...
}

void NetworkManager::runDeferredOp() {   // fires ~300ms later, off the AsyncTCP thread
    switch (_pendingOp) {
        case DeferredOp::DISCONNECT: disconnectSTA(); break;
        case DeferredOp::FORGET:     forgetSTACredentials(); break;
        case DeferredOp::RECONNECT:  reconnectSaved(); break;
        default: break;
    }
}
```

A 300ms delay is enough for the pending HTTP response to actually leave before the disruptive work runs. This pattern applies to anything that disturbs the network stack from a handler — a future "restart device" button belongs here too, not just these three WiFi actions.

## Reusing in your own project

The pieces are independent enough to lift separately:

* **AP+STA dual mode + NVS-backed provisioning** (`beginAP()`, `beginSTA()`, `startSTAProvisioning()`) — works on its own, no dependency on the rest.
* **Auto-reconnect backoff** (`attemptReconnect()`/`scheduleReconnect()`) needs `_staConnected` and `hasSavedCredentials()`, both self-contained in `NetworkManager`.
* **AP-client hold** needs the `AP_STACONNECTED`/`AP_STADISCONNECTED` counter — skip this if your project doesn't run AP+STA simultaneously (a STA-only device has no AP clients to protect, so the hold logic is unnecessary).
* **Deferred execution wrapper** is the most broadly reusable piece — any `ESPAsyncWebServer` project calling something disruptive from a handler can use the same `esp_timer`-based pattern, independent of WiFi entirely.

## Good to know

* `WiFi.mode()` is a heavy operation (rebuilds the interface) — call it once at startup (`beginAP()`), never again during runtime, especially not while a browser is actively connected to the AP.
* `attemptReconnect()`'s AP-client hold check is intentionally skipped by manual actions (`reconnectSaved()`, called from the "Connect" button) — a deliberate click is explicit user intent, not a background retry, so it shouldn't defer to a client that might just be the same user's own browser.
* A single `Preferences` namespace (`"wifi-config"`) is shared by every function that reads or writes saved credentials — a typo'd namespace string fails silently (`getString()` just returns the default), so treat that namespace name as a single source of truth if you copy this into another file.
* Association can be refused temporarily by the router itself (`wifi:Association refused temporarily, comeback time ...`) — this is normal WiFi behavior, not a bug, and is exactly the case the backoff+cap combination exists to absorb gracefully rather than get stuck failing forever.