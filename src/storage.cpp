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
#include "storage.h"
#include <stdlib.h>

Storage::Storage()
{
    this->_first = -1;
    this->_last = -1;
    memset(this->_items, 0, sizeof(this->_items));
}

void Storage::push(uint8_t *buff, uint8_t size)
{
    this->_last++;
    if (this->_last >= STORAGE_SIZE)
    {
        this->_last = 0;
    }

    if (this->_first == -1)
    {
        this->_first = this->_last;
    }
    else if (this->_first == this->_last)
    {
        this->_first++;
        if (this->_first >= STORAGE_SIZE)
        {
            this->_first = 0;
        }
    }

    memcpy(this->_items[_last].content, buff, BS);
    this->_items[_last].size = size;
}

uint8_t Storage::tryPopItem(uint8_t *pBuff)
{
    StorageItem *ptr;

    if (this->_first == -1 || this->_last == -1)
    {
        return 0;
    }

    ptr = &this->_items[this->_first];

    if (this->_first == this->_last)
    {
        this->_first = -1;
        this->_last = -1;
    }
    else
    {
        this->_first++;
        if (this->_first >= STORAGE_SIZE)
        {
            this->_first = 0;
        }
    }

    memcpy(pBuff, ptr->content, ptr->size);

    return ptr->size;
}
