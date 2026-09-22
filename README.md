# PROJECT HEADER

```
                                                     █████                                                                                ███              ███ 
                                                    ░░███                                                                                ░░░              ░░░  
  ██████   █████  ████████  █████ ███ █████  ██████  ░███████   █████   ██████  ████████  █████ █████  ██████  ████████  █████████████   ████  ████████   ████ 
 ███░░███ ███░░  ░░███░░███░░███ ░███░░███  ███░░███ ░███░░███ ███░░   ███░░███░░███░░███░░███ ░░███  ███░░███░░███░░███░░███░░███░░███ ░░███ ░░███░░███ ░░███ 
░███████ ░░█████  ░███ ░███ ░███ ░███ ░███ ░███████  ░███ ░███░░█████ ░███████  ░███ ░░░  ░███  ░███ ░███████  ░███ ░░░  ░███ ░███ ░███  ░███  ░███ ░███  ░███ 
░███░░░   ░░░░███ ░███ ░███ ░░███████████  ░███░░░   ░███ ░███ ░░░░███░███░░░   ░███      ░░███ ███  ░███░░░   ░███      ░███ ░███ ░███  ░███  ░███ ░███  ░███ 
░░██████  ██████  ░███████   ░░████░████   ░░██████  ████████  ██████ ░░██████  █████      ░░█████   ░░██████  █████     █████░███ █████ █████ ████ █████ █████
 ░░░░░░  ░░░░░░   ░███░░░     ░░░░ ░░░░     ░░░░░░  ░░░░░░░░  ░░░░░░   ░░░░░░  ░░░░░        ░░░░░     ░░░░░░  ░░░░░     ░░░░░ ░░░ ░░░░░ ░░░░░ ░░░░ ░░░░░ ░░░░░ 
                  ░███                                                                                                                                         
                  █████                                                                                                                                        
                 ░░░░░                                                                                                                                         

```
# PROJECT SUMMARY

> This project fills me with hope... and some other emotions that are weird and deeply confusing. 

How to interact with your ESP using webpage, manage its LittleFS file, connect it to the WiFi, monitor its task(s), and play games (yes, with an "s").

## Tech Stack

* PlatformIO
* WebAssembly
* Arduino + ESP-IDF (Hybrid Build)

# ROADMAP & MILESTONES

![ROADMAP_PROJECT](roadmap-link.svg)

This project is being developed in phases. Here is a roadmap of features ranging from those that have been completed to those still in the planning stages:

### Phase 1: Network Foundation & Core Systems
Focus on the device’s basic capabilities to connect to a network and be reached from it.
- [x] **Self-Service Wi-Fi Configuration:** A dedicated *dashboard* page for connecting the ESP to your home Wi-Fi without reflashing. The saved network survives reboots, and the ESP keeps running its own Wi-Fi network at the same time
- [ ] **Connection Status Indicator:** Show on the *dashboard* whether the ESP is only running its own network, or is also connected to your home Wi-Fi
- [ ] **Auto-Reconnect:** Periodically retry your saved home Wi-Fi in the background
- [ ] **Forget Wi-Fi:** Erase the saved Wi-Fi details with a click in the *browser*, no reflash needed

### Phase 2: Storage Management & Visual Interface
Focus on the user interface and the ability to manage files stored on the ESP.
- [x] **TUI-Style Interface (Terminal UI):** A lightweight, responsive, retro *web* interface that looks like a terminal, with a shared sidebar and customizable colors
- [x] **Internal File Manager:** A visual interface for browsing, previewing, uploading, creating folders, renaming, and deleting files on the ESP straight from the *browser*, with basic protection against tricky file paths (like `../`)
- [x] **Storage Capacity Information:** A visual indicator on the *dashboard* showing how much storage space is left
- [ ] **File Manager Hardening:** A list of protected files that can't be deleted by accident, plus clearer error messages when a delete fails

### Phase 3: Interactive Experiments & Entertainment
Focus on fun experiments: games that run in your browser, served by the ESP.
- [x] **Tetris:** and Snek, Playable in the *browser* and verified on the physical device with persistent high scores
- [x] **MIDI Audio System:** A retro background music player (*soundtrack*) to complement the games
- [x] **High-Performance Optimization (WebAssembly):** The games are written in C and compiled to WebAssembly, a compact format that browsers run very fast, so they play smoothly without a heavy runtime
- [ ] **More Games:** Conway's Game of Life and Minesweeper

### Phase 4: Monitoring & Tooling
Focus on observing the device and making development easier.
- [x] **Task Monitor:** A btop-style *browser* page showing every task running on the ESP: how busy each task and each CPU core is, its state, priority, which core it runs on, and how much of its working memory (stack) is left. It needs the special build setup from Getting Started, see [docs/TASK-MONITOR.md](docs/TASK-MONITOR.md) for details
- [x] **Memory Monitor:** How much memory is free, the lowest it has been since boot, and the total size
- [ ] **Local Test Environment:** A fake 1 MB file storage on your computer, so the web pages can be tested without the physical ESP

# GETTING STARTED

Five steps from zero to dashboard. Every command below can be copy-pasted. Something not working? Check [Troubleshooting](docs/TROUBLESHOOTING.md) first.

## What you need

Install these various ~sh*t~
* [VS Code](https://code.visualstudio.com/) with the [PlatformIO](https://platformio.org/platformio-ide) extension
* [Git](https://git-scm.com/)
* [Web Browser](https://www.firefox.com/)
* An ESP32 board and a USB cable that can carry data, not just power (this project uses a DevKitC variant, see the [PlatformIO configuration](/platformio.ini))

## Step 1: Get the project

```
git clone https://github.com/yoz-rn/espwebservermini.git
```

Then open the folder in VS Code (*File → Open Folder*) and wait until PlatformIO finishes loading (look at the bar at the bottom).

## Step 2: Download the missing piece (once)

Open a terminal in VS Code (*Terminal → New Terminal*) and run:

```
./setup.sh
```

This downloads a helper the project needs but can't ship inside the repo. You only do this once. (On Windows, use the *Git Bash* terminal.)

## Step 3: Name your ESP's Wi-Fi

```
cp include/secrets.EXAMPLES.h include/secrets.h
```

Open [`include/secrets.h`](/include/secrets.h) and choose a name and a password (**at least 8 characters**). The ESP creates its own Wi-Fi network with these, like a tiny router, and you'll connect to it in Step 5. Always edit `secrets.h`, never the EXAMPLES file.

## Step 4: Upload to the ESP

Plug in the board, then click these in PlatformIO, **in this order**:

1. **Build**: the first time takes a few minutes. That's normal.
2. **Upload**: sends the program to the ESP.
3. **Upload Filesystem Image** (PlatformIO sidebar → *Platform*): sends the web pages.

> [!CAUTION]
> The web pages live in the `data` folder, and it has to stay under ~1.375 MiB (1441792 bytes) or the last upload will fail. That's a storage limit of the ESP32, at least on my board anyway.
> I haven't developed the SD Card variant because of, well, budget

## Step 5: Open the dashboard

1. On your laptop or phone, connect to the Wi-Fi network from Step 3.
2. Open your favorite browser and go to:

```
http://192.168.1.254/index.html
```

3. You should see the main webpage with its pretty ASCII Art (courtesy to Letterpress from Flatpak, which is unfortunately End of Life package)

## Navigating the webpage

> [!IMPORTANT]
> Please aware that these webpage uses a highly compressed assets, so it is quite resource intense on your browser. I'll try to optimize it

1. After accessing the dashboard, scroll down slightly to see the feature, which is
    * Wi-Fi Landing Page
    * File Manager
    * Task Monitor
    * Games
2. You can read the description of each feature in their respective webpage

## Task Monitor (the fun one)

Like `btop`, but for your ESP32. It shows every FreeRTOS task, how busy each CPU core is, and how much memory is left. Open it from the dashboard and leave it running while you test your own code.

Want to understand it or reuse it in your own project? See [docs/TASK-MONITOR.md](docs/TASK-MONITOR.md).

# KNOWN LIMITATIONS

* Connecting the ESP to your home Wi-Fi (from the Wi-Fi Setup page) and the `esp32.local` address are not fully re-tested after the latest internal rebuild.
* No multi-day stability test yet. Tested with up to three separate devices at the same time.
* Website isn't autoscaled on lower resolution system. (NOT Responsive designed)

# CREDITS

This project won't come this far without the divine blessing of the internet (and a hard pill to swallow, AI)

* UI/UX
    * [system24](https://betterdiscord.app/themes/system24) theme for BetterDiscord
    * [spicetify-tui](https://github.com/AvinashReddy3108/spicetify-tui.git) theme for spicetify
    * [btop](https://github.com/aristocratos/btop) for Task Monitor

* Library
    * [webaudio-tinysynth](https://github.com/g200kg/webaudio-tinysynth) for MIDI Player

* Games
    * [Tetris](https://github.com/olzhasar/sdl-tetris), written in C and WASM
    * [Snek](https://github.com/tsoding/snake-c-wasm), written in C and WASM

* Assets
    * [Tetoris MIDI](https://onlinesequencer.net/4435806), originally by Kasane Teto
    * [Snek MIDI](https://onlinesequencer.net/1175161), originally known as Levan Polkka

* Tools
    * https://pngtosvg.com/
    * https://www.asciiart.eu/image-to-ascii