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
#include "common.h"

#ifndef STORAGE_SIZE
#define STORAGE_SIZE 10
#endif

class StorageItem
{
public:
    uint8_t content[BS];
    uint8_t size;
};

class Storage
{
    volatile int8_t _first;
    volatile int8_t _last;
    StorageItem _items[STORAGE_SIZE];

public:
    Storage();

    void push(uint8_t *buff, uint8_t size);
    uint8_t tryPopItem(uint8_t *pBuff);
};