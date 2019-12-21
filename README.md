# esp32-oled-hud
A little HUD for my ESP32 OLED to connect to my ReadyNAS

## Setup

Install the libraries using the guide provided [here](https://github.com/FablabTorino/AUG-Torino/wiki/Wemos-Lolin-board-(ESP32-with-128x64-SSD1306-I2C-OLED-display))

You'll need SSD1306 and GFX libs.

## Flashing

Setup ESP32 by adding the board manager:

https://dl.espressif.com/dl/package_esp32_index.json

And select `WEMOS LOLIN32` and set the clock speed to 80 MHz.

## ReadyNAS setup

This'll only work with ReadyNAS. Only tested on RN104 FW 6.10.2.

Make a copy of `credentials.h` from the template file, and fill it in with your credentials.
