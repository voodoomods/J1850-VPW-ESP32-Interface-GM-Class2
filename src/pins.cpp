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
#include "pins.h"

bool Pin::isEmpty() const
{
    return this->_pin == -1;
}

void Pin::write(uint8_t val)
{
	digitalWrite(this->_pin, val);  
}

uint8_t Pin::read()
{
	return digitalRead(this->_pin);
}

Pin ::~Pin()
{
    //detachInterrupt(this->_pin);
}

Pin::Pin()
{
    this->_mode = PIN_MODE_INPUT;
    this->_pin = -1;
}

Pin::Pin(uint8_t pin, PIN_MODES mode)
{
    // Configure the pin based on the specified mode
	if (mode == PIN_MODE_INPUT) {
		pinMode(pin, INPUT);
	} else if (mode == PIN_MODE_INPUT_PULLUP) {
		pinMode(pin, INPUT_PULLUP);
	} else if (mode == PIN_MODE_OUTPUT) {
		pinMode(pin, OUTPUT);
		digitalWrite(pin, LOW);  // Ensure it starts in a known state (LOW)
	}

    this->_mode = mode;
    this->_pin = pin;
}
