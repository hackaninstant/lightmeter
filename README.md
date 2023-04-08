# lightmeter
A Lightmeter for photographers, based on Arduino for the Nano 168.

This fork slashed down the code to fit into an ATMEGA 168 which only has 16K memory. The original required 27K.
Features that were removed from master:

* ND factor
* flash metering
* shutter speed rounding
* aperture rounding
* graphics display

Code modifications: ISO minimum: .8, using array instead of conditionals
Uses SSD1306Ascii.h instead of Adafruit to save space

Components:
1. Arduino NANO v.3 ATMEGA 168 (16K)
2. BH1750 light sensor https://www.banggood.com/custlink/mKDv2Ip1dr or https://www.banggood.com/custlink/GvGvnRNN0e
3. SSD1306 128*64 OLED IC2 Display
5. 50x70 PCB https://www.banggood.com/custlink/KvvvnybQAP
6. AAA battery Holder https://www.banggood.com/custlink/vK3KsynANN

Thanks @morozgrafix https://github.com/morozgrafix for creating schematic diagram for this device.

The lightmeter based on Arduino as a main controller and BH1750 as a metering cell. Information is displayed on SSD1306 OLED display. The device is powered by 2 AAA batteries.

Functions list:

* Ambient light metering
* Aperture priority
* Shutter speed priority
* ISO range .8 - 8000
* Aperture range 1.0 - 3251
* Shutter speed range 1/10000 - 133 sec
* Displays amount of light in Lux.
* Displays exposure value, EV
* Recalculating exposure pair while one of the parameter changing
* Battery information
* Power 2xAAA LR03 batteries

Detailed information on my site: https://www.pominchuk.com/lightmeter/
