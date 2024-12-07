/*************************************************************************
**  ESP32 J1850VPW Interface (GM Class 2 Serial)
**  
**  Former Project J1850-VPW-Arduino-Transceiver-Library
**  https://github.com/matafonoff/J1850-VPW-Arduino-Transceiver-Library
**  by Stepan Matafonov -> matafonoff -> xelb.ru
**
**  Thank you Stephan for your brilliant work sir
**
**  Released under Microsoft Public License
**
**************************************************************************/
#pragma once

#include <Arduino.h>

#define BS               12
#define MAX_DATA_LEN    (BS - 1)