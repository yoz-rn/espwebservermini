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

# ROADMAP & MILESTONES

![ROADMAP_PROJECT](roadmap-link.svg)

This project is being developed in phases. Here is a roadmap of features ranging from those that have been completed to those still in the planning stages:

### Phase 1: Network Foundation & Core Systems
Focus on the device’s basic capabilities to connect to and be accessed via a network.
- [x] **Self-Service Wi-Fi Configuration:** A dedicated *dashboard* page for connecting the device to a Wi-Fi network without reflashing. Credentials are stored in NVS and survive reboots, with AP and STA modes running side by side
- [ ] **Connection Status Indicator:** Show on the *dashboard* whether the device is in AP-only or STA-connected mode
- [ ] **STA Auto-Reconnect:** Periodically retry the saved network in the background
- [ ] **Forget Wi-Fi:** Clear the saved credentials from the *browser*, no reflash needed

### Phase 2: Storage Management & Visual Interface
Focus on the user interface and the ability to manage files in the device’s memory.
- [x] **TUI-Style Interface (Terminal UI):** A lightweight, responsive, retro *web* interface with a terminal-style look, a shared sidebar, and customizable colors
- [x] **Internal File Manager:** A visual interface for browsing, previewing, uploading, creating folders, renaming, and deleting files directly from the *browser*, with path traversal protection
- [x] **Storage Capacity Information:** A visual indicator on the *dashboard* to monitor the remaining available storage space
- [ ] **File Manager Hardening:** A protected files list for critical paths, plus clearer error messages for failed deletes
- [ ] **Partition Awareness:** Compare the LittleFS partition against the real flash capacity using `partitions.csv`

### Phase 3: Interactive Experiments & Entertainment
Focus on advanced technology experiments to present interactive applications.
- [x] **Tetris:** Playable in the *browser* and verified on the physical device
- [x] **Snek:** WASM backend is done, frontend page still needs to be integrated
- [x] **8-bit Audio System:** A retro background music player (*soundtrack*) to complement the games
- [x] **Persistent High Scores:** Scores are saved on the device and shown in the game UI
- [x] **High-Performance Optimization (WebAssembly):** Freestanding C compiled to WASM, so games run smoothly without a heavy runtime
- [ ] **Tetris Polish:** Next-piece preview, pause and reset buttons
- [ ] **More Games:** Conway's Game of Life and Minesweeper

### Phase 4: Monitoring & Tooling
Focus on observing the device and making development easier.
- [ ] **Task Monitor:** Real-time FreeRTOS task stats streamed to the *browser*
- [ ] **Local Test Environment:** An emulated 1 MB LittleFS so the frontend can be tested without the physical device

# GETTING STARTED

## Prerequisites

Install these various ~sh*t~
* [VS Code](https://code.visualstudio.com/) + [PlatformIO](https://platformio.org/platformio-ide)
* [Web Browser](https://www.firefox.com/)
* ESP32 DevKit Board (This project uses a devkitc variant. See:[platformIO configuration](/platformio.ini))

## Cloning the project
1. Cloning the repository

    ```
    git clone https://github.com/yoz-rn/espwebservermini.git
    ```
2. Configuring the network credentials
    ```
    cp include/secrets.EXAMPLES.h include/secrets.h
    ```

    Navigate to the [secrets.h file](/include/secrets.h) to configure the ESP via Wi-Fi. Remember to ALWAYS edit the `secrets.h`. Currently there are two methods available:
    * AP mode, treat the ESP as a router / access point
    * Station mode, treat the ESP as a device connected to the home network

## Building and uploading the code

1. Building the project

    Use the `Build` button or copy the command below
    

2. Uploading the project

    To flash the main code and embed it to the ESP, Use the `Upload` button

3. Flash the webpage

    To upload the webpage, Use the `Upload Filesystem Image` button. This will upload all file and folders in the `data` folder to the esp LittleFS partition.

> [!CAUTION]
> Due to limitations of ESP32 Flash Memory, make sure the `/data` directory is always under ~1,375 MiB (1441792 bytes), at least in my board anyway.
> I havent't developed the SD Card variant because of, well budget

## Access the dashboard

1. Connect to your ESP using its AP Mode
2. Copy the link below (by default, its IP is 192.168.1.254)

    ```
    http://192.168.1.254/index.html
    ```

3. You should see the main webpage with its pretty ASCII Art (courtesy to Letterpress from Flatpak, which is unfortunately is End of Life package)

## Navigating the webpage

1. After accessing the dashboard, scroll down slightly to see the feature, which is
    * Wi-Fi Landing Page
    * File Manager
    * Task Monitor
    * Games
2. You can read the description of each feature in their respective webpage

# CREDITS

This project won't come this far without the divine blessing of the internet (and a hard pill to swallow, AI)

* UI/UX
    * [system24](https://betterdiscord.app/themes/system24) theme for BetterDiscord
    * [spicetify-tui](https://github.com/AvinashReddy3108/spicetify-tui.git) theme for spicetify

* Library
    * [webaudio-tinysynth](https://github.com/g200kg/webaudio-tinysynth) for MIDI Player

* Games
    * [Tetris](https://github.com/olzhasar/sdl-tetris)
    * [Snek](https://github.com/tsoding/snake-c-wasm)

* Assets
    * [Tetoris MIDI](https://onlinesequencer.net/4435806) by [helo_dayo](https://onlinesequencer.net/members/129097) 

* Tools
    * https://pngtosvg.com/
    * https://www.asciiart.eu/image-to-ascii