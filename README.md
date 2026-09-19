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
- [x] **Self-Service Wi-Fi Configuration:** A dedicated *dashboard* page for easily connecting devices to a Wi-Fi network

### Phase 2: Storage Management & Visual Interface
Focus on the user interface and the ability to manage files in the device’s memory.
- [x] **TUI-Style Interface (Terminal UI):** A lightweight, responsive, retro *web* interface with a terminal-style look and customizable colors
- [x] **Internal File Manager:** A visual interface for viewing, uploading, creating folders, and deleting files directly from the *browser*.
- [x] **Storage Capacity Information:** A visual indicator on the *dashboard* to monitor the remaining available storage space

### Phase 3: Interactive Experiments & Entertainment
Focus on advanced technology experiments to present interactive applications.
- [x] **Mini-Game Integration:** Embed classic games like Tetris and Snake that can be played directly in the *browser*
- [x] **8-bit Audio System:** Add a retro background music player (*soundtrack*) to complement the game
- [x] **High-Performance Optimization (WebAssembly):** Implementing WebAssembly technology so that games and animations run extremely smoothly on users’ devices

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
    > Due to limitations of ESP32 Flash Memory, make sure the `/data` directory is always under ~1,375 MB (1441792 bytes), at least in my board anyway.
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