# Troubleshooting

Stuck on a step of **Getting Started**? Find your step below. If your problem isn't listed, open an issue and paste the exact error message.

> **Tip:** most problems show up in one of two places: the red text at the bottom of the **Build** output, or the **Serial Monitor** (PlatformIO sidebar → *Monitor*, speed 115200). Copy that text. It is the fastest way to get help.

## Why is there an extra setup step?

Normal Arduino projects hide FreeRTOS (the ESP32's task scheduler) inside precompiled files. The **Task Monitor** needs to read statistics from it, so this project builds Arduino together with ESP-IDF (the official ESP32 framework) instead. The price is one extra download (`setup.sh`) and a custom storage layout (`partitions.csv`). Nothing you need to touch, but it explains a few of the messages below.

---

## Step 2: `./setup.sh` doesn't work

| What you see | What it means | Try this |
|---|---|---|
| `Permission denied` | The script isn't marked as runnable on your computer | Run `bash setup.sh` instead |
| `git: command not found` | Git isn't installed | Install [Git](https://git-scm.com/), then reopen VS Code |
| Errors about `.sh` on Windows | Windows can't run `.sh` files directly | Open the terminal as **Git Bash** (it comes with Git) and run `./setup.sh` again |
| Download errors | No internet, or a firewall is blocking GitHub | Check your connection and run it again |

Success looks like a new folder called `components/esp_littlefs`. **Don't rename it**: the build looks for that exact name.

## Step 3: `secrets.h` problems

* **Build says `secrets.h: No such file`**: you skipped the `cp` command. Run it from the project folder.
* **The ESP's Wi-Fi network never shows up**: the password must be **at least 8 characters**. A shorter one stops the ESP from starting its network, and the Serial Monitor shows an error at boot.

## Step 4: Build and upload problems

**`#error ACTIVATE CONFIG_FREERTOS_USE_TRACE_FACILITY IN sdkconfig.defaults`** (or the `RUN_TIME_STATS` version)
The Task Monitor needs some FreeRTOS statistics switched on. They are on in `sdkconfig.defaults`, so you only see this if that file was changed. Restore the file from the repo, then follow [Changing build settings](#changing-build-settings-advanced) below.

**`esp_littlefs.h: No such file or directory`** or **`undefined reference to fs::LittleFSFS`**
Step 2 wasn't done, or the folder was renamed. Check that `components/esp_littlefs` exists; if not, run `./setup.sh`.

**`undefined reference to app_main`**
Make sure `sdkconfig.defaults` contains `CONFIG_AUTOSTART_ARDUINO=y`, then follow [Changing build settings](#changing-build-settings-advanced).

**The first build takes very long**
Normal. It compiles the whole ESP-IDF from source (a few minutes). Changing build settings triggers another full build.

**Upload says "Failed to connect" or can't find the board**
* Use a USB cable that carries data (some cables only charge).
* On some boards you must hold the **BOOT** button while the upload says `Connecting...`.

**Upload Filesystem Image fails or complains about size**
The `data` folder is too big. Keep it under 1.375 MiB (1441792 bytes).

**Serial Monitor shows `Mounting LittleFS failed! Error: 261`**
The ESP can't find the storage area for the web pages. This usually means the steps were done in the wrong order. Redo them like this: **Upload** first (it also writes the storage layout), then **Upload Filesystem Image**.

## Step 5: I can't open `http://192.168.1.254/index.html`

* Make sure your laptop or phone is connected to the **ESP's** Wi-Fi (the name you chose in `secrets.h`), not your home Wi-Fi.
* Phones often say "No internet" and quietly hop back to mobile data or another network. Tap the ESP's network and choose to stay connected, or turn off mobile data.
* Check the Serial Monitor for the line that shows the Access Point address, in case it differs.
* Seeing an old version of a page after re-uploading? Hard reload: `Ctrl+Shift+R` (`Cmd+Shift+R` on Mac).

---

## Changing build settings (advanced)

* `sdkconfig.defaults` is the **source of truth** for build options. The generated `sdkconfig.<env>` file is git-ignored.
* If you change an option: edit `sdkconfig.defaults`, **delete** the generated `sdkconfig.<env>` file, then build again so it is regenerated. Don't edit both; the generated file wins when it exists.
* The storage layout is in `partitions.csv` (based on Arduino's default one) and is referenced from `platformio.ini`. The web-page storage is 1441792 bytes.
* Defaults you may notice compared with a plain Arduino build:
  * The CPU runs at **160 MHz** (Arduino usually uses 240 MHz).
  * The task watchdog is on (5 seconds) and also watches both idle tasks. Blocking a core for more than 5 seconds prints a warning; it does not reset the board.
* Coming from an older Arduino-only version of this project? The storage layout changed, so you may need to enter your Wi-Fi credentials again.