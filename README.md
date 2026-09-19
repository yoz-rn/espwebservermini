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

# ROADMAP

![ROADMAP_PROJECT](roadmap-link.svg)

- [x] Wi-Fi Landing Page Dashboard
- [x] LittleFS File Manager
- [x] TUI-Style webpage
- [ ] Bikin game (tetris + snek (+ musik 8 bit))
- [ ] Use WebAssembly if possible
- [ ] Fix Task Manager

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

    > [!WARNING]
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
    * Games (yes, with an "S")
2. You can read the description of each feature in their respective webpage

# CREDITS

This project won't come this far without the divine blessing of the internet (and of course AI)

* UI/UX
    * [system24](https://betterdiscord.app/themes/system24) theme for better discord
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