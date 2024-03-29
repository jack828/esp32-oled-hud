# esp32-oled-hud
A little HUD for my ESP32 OLED to connect to my ReadyNAS

## Warning

Whilst it would be nice to be able to access your ReadyNAS from outside your home network (hence the ability to support up to 3 SSID's), I really would not recommend allowing external access of the admin page of your NAS, unless you really know what you are doing.

## Setup

Install the libraries using the guide provided [here](https://github.com/FablabTorino/AUG-Torino/wiki/Wemos-Lolin-board-(ESP32-with-128x64-SSD1306-I2C-OLED-display))

You'll need these libraries from the Arduino library manager:
 - SSD1306
 - Adafruit GFX
 - NTPClient

## Flashing

Setup ESP32 by adding the board manager:

https://dl.espressif.com/dl/package_esp32_index.json

And select `WEMOS LOLIN32` and set the clock speed to 80 MHz.

## ReadyNAS setup

This'll only work with ReadyNAS. Only tested on RN104 FW 6.10.2.

Make a copy of `credentials.h` from the template file, and fill it in with your credentials.
