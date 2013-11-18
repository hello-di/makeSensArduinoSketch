Make Sens Arduino Source Code
===================

This repo is the Arduino source code for the wireless sensor communication system developed by [Di Jiang](http://deene.github.io) and [Carl Zandén](mailto:carl.zanden@chalmers.se). The project was originally called Bodify but later we found this MakeSens name suit better with the goal of this project. Therefore the source code folder name is bodify at this moment, yet it will be replaced with the new project name soon.

This Arduino project is based on the following open-source Arduino libraries:
- [Adafruit LSM303](https://github.com/adafruit/Adafruit_LSM303)
- [Redbear Arduino BLE shield[(https://github.com/RedBearLab/BLEShield)

Since the Redbear Arduino BLE is still in development phase and its support for customized Bluetooth Low Energy (BLE) funtion is limited at the moment, a modification of the BLE shield and the library has been done according to a [post](https://redbearlab.zendesk.com/entries/23440476-Getting-Started) by Redbear, which is the provider of the BLE chip on the shield board.

### Note

This library has been written only to enable the most basic BLE function - to send the sensor data to a Mac center manager app. Both the Arduino and the Mac code are under heavy development work, and no production release is going to be possible within next half a year by estimation.
