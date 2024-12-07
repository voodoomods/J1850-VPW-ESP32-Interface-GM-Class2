/*************************************************************************
**  ESP32 J1850VPW Interface (GM Class 2 Serial)
**  by Nicolas Houghtaling @ VoodooMods
**
**  Former Project J1850-VPW-Arduino-Transceiver-Library
**  https://github.com/matafonoff/J1850-VPW-Arduino-Transceiver-Library
**  by Stepan Matafonov -> matafonoff -> xelb.ru
**
**  Thank you Stephan for your brilliant work sir
**
**  Released under Microsoft Public License
**
**  contact: admin@voodoomods.com
**  homepage: voodoomods.com
**************************************************************************/
#pragma once
#include <Arduino.h>
#include "pins_arduino.h"

enum PIN_CHANGE {
    PIN_CHANGE_BOTH = 3,
    PIN_CHANGE_RISE = 1,
    PIN_CHANGE_FALL = 2
};

enum PIN_MODES {
    PIN_MODE_INPUT = INPUT,
    PIN_MODE_INPUT_PULLUP = INPUT_PULLUP,
    PIN_MODE_OUTPUT = OUTPUT,
};

class Pin {
    

    PIN_MODES _mode;
public:
	uint8_t _pin;
    Pin();
    Pin(uint8_t pin, PIN_MODES mode);
    ~Pin();
public:
    void write(uint8_t val);
    uint8_t read();

    bool isEmpty() const;
};


