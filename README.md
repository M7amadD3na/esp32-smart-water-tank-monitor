# ESP32 Smart Water Tank Monitor

Measures water level using an ultrasonic sensor, displays a local web dashboard, drives status LEDs and a buzzer, and uploads readings to ThingSpeak.

## Technologies

ESP32, ultrasonic sensor, LEDs, buzzer, web dashboard, ThingSpeak

## Requirements

Arduino core for ESP32 (`WiFi.h`, `WebServer.h`).

## Hardware and setup notes

Hardware pins in the sketch: TRIG GPIO14, ECHO GPIO25, LEDs GPIO26/27/33, and buzzer GPIO32. Water-level calibration is set using `DIST_FULL` and `DIST_EMPTY` (15 cm and 80 cm). ESP32 GPIO pins are **not 5 V tolerant**; use suitable voltage conditioning if the ultrasonic ECHO output is 5 V.

## Run

1. Install the Arduino IDE and the required board support and libraries.
2. Open `Smart_Water_Tank_Monitor.ino` from this folder.
3. Review pin assignments, the target board, and any network settings.
4. Replace placeholders (such as `YOUR_THINGSPEAK_WRITE_API_KEY`) with **your own settings locally**. Never commit real passwords or API keys.
5. Compile and upload to compatible hardware; monitor serial output where relevant.

> Note: This portfolio package is a cleaned copy of an existing project sketch, not a claim that it was retested on hardware or security-hardened.
