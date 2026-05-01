# Smart Road Lighting Sensor (SRLS)

### Overview
The Smart Road Lighting Sensor (SRLS) is a decentralised, adaptive street lighting control system. Built around the ESP32-C6 microcontroller, the system integrates 24GHz Doppler microwave radar and ambient light sensing to dynamically command commercial DALI-2 luminaires. 

By prioritising a distributed master/slave architecture over cloud-dependent processing, the SRLS aims to drastically reduce energy consumption in road lighting installations while making sure to allow the customisation of various variable in order to make sure that all deployments adhere to the BS EN 13201 lighting standards.

### Key Features
* **Predictive Traffic Adaptation:** Utilises a BGT24LTR11 24GHz Doppler radar to detect vehicle motion, instantly scaling DALI luminaires to 100% brightness.
* **Software-Defined Fading:** Bypasses OEM luminaire limitations with a RTOS-driven 10-second linear fade out, preventing jarring light cut-offs.
* **State-Aware Hysteresis:** Implements a self-blinding algorithmic override to prevent the luminaire's artificial light output from falsely triggering its own daytime shut-off thresholds.
* **ESP-NOW Communication:** Employs a zero-infrastructure, low-latency ESP-NOW broadcast network to transmit node data to a secondary node without interrupting the primary 200ms DALI control loop.

### Hardware Requirements
* **Microcontroller:** ESP32-C6-Pico (Primary Sensor Node & Secondary Telemetry Gateway)
* **Radar Module:** Grove BGT24LTR11 (24GHz Doppler)
* **Light Sensor:** Adafruit TSL2591 (High Dynamic Range I2C)
* **Lighting Interface:** Waveshare DALI-2 Expansion Module for ESP32-C6-Pico
* **Power:** Isolated AC-DC Step-Down Converter (Mains to 5V/3.3V Logic)

### Software Dependencies
This project is built using the Arduino Core for ESP32. The following libraries are required to compile the firmware:
* `BGT24LTR11.h` - Doppler Radar Object Abstraction
* `Adafruit_TSL2591.h` - I2C Ambient Light Acquisition
* `esp_now.h` & `WiFi.h` - ESP-NOW Protocol Stack
* `WS_DALI.h` - Waveshare DALI expansion board communication protocols
* *Note: Additional secondary software dependencies in this repo are also required for compiling. Just make sure to download all the header files in the repo and save them in the same location as the main scripts to ensure successful compilation*

### System Architecture
The main code is divided into the two programs:
* `SRLS_main.ino` - Contains the main control loop, sensor polling, DALI actuation, and broadcast transmission logic.
* `SRLS_receiver.ino` - Contains the asynchronous ESP-NOW receive callback and Serial Native USB formatting logic for dashboard integration.

### Author
**Sam Granger** - BEng Electronic Engineering
*The University of Manchester*
