# TINA-3
GPS based on ESP32 with TFT SPI 2.4 + SD card

This is project, where i wanted to do GPS based on ESP32. This is next version of my project.
Saved data can be shown on google maps by web application. You need to put your Google API to code.

Data must by multiplicated in ESP because of precision of location. When are data saving to SD card ESP and Arduino saving with double precision after comma -> you need to multiply values by 1000.

You must use hardware SPI, if you use software SPI the card of display will not work.
Code is in development.


<li>https://github.com/mikalhart/TinyGPSPlus/releases TinyGPS++ library 
<li>https://github.com/nhatuan84/esp32-micro-sdcard mySD library
