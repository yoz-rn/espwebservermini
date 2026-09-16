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

How to interact with your ESP using webpage

## Tech Stack

* PlatformIO
* 

# ROADMAP

![ROADMAP_PROJECT](roadmap-link.svg)

- [x] Wi-Fi Landing Page Dashboard
- [ ] LittleFS File Manager
- [ ] TUI-Style webpage
- [ ] Use WebAssembly if possible
- [ ] Fix Task Manager
- [ ] Bikin game 

# GETTING STARTED

## Prerequisites

* [VS Code](https://code.visualstudio.com/) + [PlatformIO](https://platformio.org/platformio-ide)
* [Web Browser](https://www.firefox.com/)

## Cloning the project
1. Cloning the repository

    ```
    git clone https://github.com/yoz-rn/espwebservermini.git
    ```
2. Configuring the network credentials

    On your root project,
    ```
    mv include/secrets.EXAMPLES.h include/secrets.h
    ```

    Navigate to the [secrets.h file](/include/secrets.h) to configure the ESP via Wi-Fi. Currently there are two methods available:
    * AP mode, treat the ESP as a router / access point
    * Station mode, treat the ESP as a device connected to the home network


3. Building the project

    Use the `Build` button or copy the command below
    ```
    platformio run # didn't work on my laptop, I'll try a different approach (much) later
    ```

4. Uploading the project

    To flash the main code and embed it to the ESP, Use the `Upload` button

5. Flash the web

    To upload the webpage, Use the `Upload Filesystem Image` button. This will upload all file and folders in the `data` folder to the esp LittleFS partition.

5. 