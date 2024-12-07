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
#include "j1850vpw.h"
#include <pins_arduino.h>
#include <stdlib.h>

//#define BitBanger_LED_PIN 13

#define MAX(a, b) ((a) < (b) ? (b) : (a))

#define MAX_uS ((unsigned long )-1L)

#define RX_SOF_MIN (163)
#define RX_SOF_MAX (239)

#define RX_EOD_MIN (164) // minimum end of data time
#define RX_EOF_MIN (239) // minimum end of data time

#define RX_SHORT_MIN (34) // minimum short pulse time
#define RX_SHORT_MAX (96) // maximum short pulse time
#define RX_SHORT_IGNORE (5) // Ignore pulses shorter than this to reduce noise

#define RX_LONG_MIN (97)  // minimum long pulse time
#define RX_LONG_MAX (163) // maximum long pulse time

#define RX_ARBITRATION_TOL (10) // Tolerance for shorter passive pulses from other modules during arbitration
#define PROPOGATION_DELAY  (10)  //Propogation delay from driving output Passive to checking input follows 


#define RX_EOD_MAX (239) // maximum end of data time

#define RX_PULSE_MAX (3000)

#define TX_SHORT (64) // Short pulse nominal time
#define TX_LONG (128) // Long pulse nominal time
#define TX_SOF (200)  // Start Of Frame nominal time
#define TX_EOD (200)  // End Of Data nominal time
#define TX_EOF (280)  // End Of Frame nominal time

#define IS_BETWEEN(x, min, max) (x >= min && x <= max)

uint8_t crc(uint8_t *msg_buf, int8_t nbytes)
{	
    uint8_t crc_reg = 0xff, poly, byte_count, bit_count;
    uint8_t *byte_point;
    uint8_t bit_point;

    for (byte_count = 0, byte_point = msg_buf; byte_count < nbytes; ++byte_count, ++byte_point)
    {
        for (bit_count = 0, bit_point = 0x80; bit_count < 8; ++bit_count, bit_point >>= 1)
        {
            if (bit_point & *byte_point) // case for new bit = 1
            {
                if (crc_reg & 0x80)
                    poly = 1; // define the polynomial
                else
                    poly = 0x1c;
                crc_reg = ((crc_reg << 1) | 1) ^ poly;
            }
            else // case for new bit = 0
            {
                poly = 0;
                if (crc_reg & 0x80)
                    poly = 0x1d;
                crc_reg = (crc_reg << 1) ^ poly;
            }
        }
    }
    return ~crc_reg; // Return CRC
}

#define UNKNOWN_PIN 0xFF

J1850_ERRORS J1850VPW::handleErrorsInternal(J1850_Operations op, J1850_ERRORS err)
{	
    if (err != J1850_OK)
    {
        onErrorHandler errHandler = __errHandler;
        if (errHandler)
        {
            errHandler(op, err);
        }
    }

    return err;
}

bool J1850VPW::isReadonly() const {
    return __txPin.isEmpty();
}

void J1850VPW::RxChanged()
{
    uint8_t curr = __rxPin.read();
	
    _currState = curr;
    curr = !curr;

    unsigned long  now = micros();
    unsigned long  diff;

    if (now < _lastChange)
    {
        // overflow occured
        diff = now + (MAX_uS - _lastChange);
    }
    else
    {
        diff = now - _lastChange;
    }

    if (diff < RX_SHORT_IGNORE) //We filter out some noise for very short pulses around the transition
    {
        return;
    }

    _lastChange = now;
	//Serial.println(diff);
    if (diff < RX_SHORT_MIN)
    {
        // too short to be a valid pulse. Data error
        _sofRead = false;
        handleErrorsInternal(J1850_Read, J1850_ERR_PULSE_TOO_SHORT);
        return;
    }

    if (!_sofRead)
    {
        if (_sofRead = (curr == ACTIVE && IS_BETWEEN(diff, RX_SOF_MIN, RX_SOF_MAX)))
        {
            _byte = 0;
            _bit = 0;
            msg_buf = _buff;
            *msg_buf = 0;
            _IFRDetected = false;
        }
        else
        {
            handleErrorsInternal(J1850_Read, J1850_ERR_PULSE_OUTSIDE_FRAME);
        }
        return;
    }

    *msg_buf <<= 1;
    if (curr == PASSIVE)
    {
        if (diff > RX_EOF_MIN)
        {
            // data ended - copy package to buffer
			//digitalWrite(BitBanger_LED_PIN, LOW);//packet ready
            _sofRead = false;

            if (!_IFRDetected)
            {
                    onFrameRead();
            }
            return;
        }
        if (!_IFRDetected && IS_BETWEEN(diff, RX_EOD_MIN, RX_EOD_MAX))
        {
            // data ended and IFR detected - set flag to ignore the incoming IFR and flag error
            _IFRDetected = true;
            handleErrorsInternal(J1850_Read, J1850_ERR_IFR_RX_NOT_SUPPORTED);
            onFrameRead();
            return;
        }
        if (!_IFRDetected && IS_BETWEEN(diff, RX_LONG_MIN, RX_LONG_MAX))
        {
            *msg_buf |= 1;
        }
    }
    else if (!_IFRDetected)
    {
        if (diff <= RX_SHORT_MAX)
        {
            *msg_buf |= 1;
        }
    }
    if(!_IFRDetected)
    {
        _bit++;
        if (_bit == 8)
        {
            _byte++;
            msg_buf++;
            *msg_buf = 0;
            _bit = 0;
        }
    }    
    if (_byte == BS)
    {
		//digitalWrite(BitBanger_LED_PIN, LOW);//packet ready
        _sofRead = false;
        if (!_IFRDetected)
        {
            onFrameRead();
        }
    }
}

J1850VPW::J1850VPW() 
: ACTIVE(LOW)
, PASSIVE(HIGH)
, RX(-1)
, __rxPin(Pin())
, __txPin(Pin())
, _lastChange(micros())
, _sofRead(false)
, _currState(ACTIVE)
, _bit(0)
, _byte(0)
, msg_buf(NULL)
, _storage(Storage())
, __errHandler(NULL)
{
    listenAll();
}

J1850VPW* J1850VPWFriend::instance = nullptr;

void J1850VPWFriend::__handleRnChange() {
    instance->RxChanged();
}

J1850VPW* J1850VPW::setActiveLevel(uint8_t active)
{
    if (active == LOW)
    {
        ACTIVE = LOW;
        PASSIVE = HIGH;
    }
    else
    {
        ACTIVE = HIGH;
        PASSIVE = LOW;
    }

    return this;
}

J1850VPW* J1850VPW::init(uint8_t rxPin, uint8_t txPin)
{
	J1850VPWFriend::instance = this;
    init(rxPin);

    __txPin = Pin(txPin, PIN_MODE_OUTPUT);
    __txPin.write(PASSIVE);
	
	//pinMode(BitBanger_LED_PIN, OUTPUT);
	//digitalWrite(BitBanger_LED_PIN, HIGH);
    return this;
}

J1850VPW* J1850VPW::initTx(uint8_t txPin)
{
    __txPin = Pin(txPin, PIN_MODE_OUTPUT);
    __txPin.write(PASSIVE);
    return this;
}

J1850VPW* J1850VPW::killTx(uint8_t txPin)
{
	pinMode(txPin, OUTPUT);
    digitalWrite(txPin, HIGH);	
	//__txPin = Pin(txPin, OUTPUT);
    return this;
}

J1850VPW* J1850VPW::init(uint8_t rxPin) 
{	
	attachInterrupt(digitalPinToInterrupt(rxPin), &J1850VPWFriend::__handleRnChange, CHANGE);
    __rxPin = Pin(rxPin, PIN_MODE_INPUT_PULLUP);	
    _currState = __rxPin.read();
	
    return this;
}

J1850VPW* J1850VPW::onError(onErrorHandler errHandler)
{
    __errHandler = errHandler;
    return this;
}

int8_t J1850VPW::tryGetReceivedFrame(uint8_t *pBuff, bool justValid /*= true*/)
{
    uint8_t size;
    bool crcOK = true;

    while (true)
    {		
        size = _storage.tryPopItem(pBuff);
		
        if (!size)
        {
            return 0;
        }
        if (crc(pBuff, size - 1) != pBuff[size - 1])
        {
            crcOK = false;
            handleErrorsInternal(J1850_Read, J1850_ERR_CRC);
        }		
        if (!justValid || crcOK)
        {
            break;
        }		
    }
	//digitalWrite(BitBanger_LED_PIN, HIGH);
    return size;
}

void J1850VPW::onFrameRead()
{	
    if (!IS_BETWEEN(_byte, 2, BS))
    {
        return;
    }

    uint8_t *pByte, bit;
    
    pByte = getBit(_buff[0], &bit);
    if (pByte && (*pByte & (1 << bit)) == 0) {
        _storage.push(_buff, _byte);
    }
}

J1850VPW* J1850VPW::listenAll() {
    memset(_ignoreList, 0, sizeof(_ignoreList));
    return this;
}

J1850VPW* J1850VPW::listen(uint8_t *ids) {
    uint8_t *pByte, bit;
    while (*ids) {
        pByte = getBit(*ids, &bit);

        if (pByte) {
            *pByte &= ~(1 << bit);
        }

        ids++;
    }
    return this;
}

J1850VPW* J1850VPW::ignoreAll() {
    memset(_ignoreList, 0xff, sizeof(_ignoreList));
    return this;
}

J1850VPW* J1850VPW::ignore(uint8_t *ids) {
    uint8_t *pByte, bit;
    while (*ids) {
        pByte = getBit(*ids, &bit);

        if (pByte) {
            *pByte |= 1 << bit;
        }

        ids++;
    }
    return this;
}

uint8_t* J1850VPW::getBit(uint8_t id, uint8_t *pBit) {
    if (!id) {
        *pBit = 0xff;
        return NULL;
    }

    *pBit = id % 8;
    return &(_ignoreList[id / 8]);
}

// NB! Performance critical!!! Do not split
uint8_t J1850VPW::send(uint8_t *pData, uint8_t nbytes, int16_t timeoutMs /*= -1*/)
{
    if (isReadonly())
    {
        return handleErrorsInternal(J1850_Write, J1850_ERR_RECV_NOT_CONFIGURATED);
    }

    J1850_ERRORS result = J1850_OK;
    static uint8_t buff[BS];
    memcpy(buff, pData, nbytes);
    buff[nbytes] = crc(buff, nbytes); //CRC added here
    nbytes++;
    
	/*Serial.print("QQQ: ");
    for (int q = 0; q < nbytes; q++) {
         uint8_t w = buff[q];
         if (w < 0x10) {
             Serial.print('0');
         }
         Serial.print(String(w, HEX));
     }
     Serial.println();*/
    

    uint8_t *msg_buf = buff;
    unsigned long now;

    // wait for idle
    if (timeoutMs >= 0)
    {
        now = micros();
        unsigned long  start = now;
        timeoutMs *= 1000; // convert to microseconds
        while (micros() - now < TX_EOF)
        {
            if (__rxPin.read() == ACTIVE)
            {
                now = micros();
            }

            if (micros() - start > timeoutMs)
            {
                result = J1850_ERR_BUS_IS_BUSY;
                goto stop;
            }
        }
    }

    // SOF
    //pauseInterrupts;
	detachInterrupt(digitalPinToInterrupt(__rxPin._pin));
    __txPin.write(ACTIVE);
    now = micros();
    while (micros() - now < TX_SOF)
        ;

    // send data
    do
    {
        uint8_t temp_byte = *msg_buf; // store byte temporary
        uint8_t nbits = 8;
        unsigned long  delay;
        while (nbits--) // send 8 bits
        {
            if (nbits & 1) // start allways with passive symbol
            {
                delay = (temp_byte & 0x80) ? TX_LONG : TX_SHORT; // send correct pulse lenght
                __txPin.write(PASSIVE);                          // set bus active
                now = micros();
                delayMicroseconds(PROPOGATION_DELAY);            // Allow time for RX to follow TX for fast processors
                while (micros() - now < delay)
                {
                    if (__rxPin.read() == ACTIVE)
                    {
                        if (delay - (micros() - now)  < RX_ARBITRATION_TOL) //Arbitration not lost
                        {
                            now = micros() - delay - 1; //resync to faster module
                        } else //We lost arbitration so drop out
                        {
                            result = J1850_ERR_ARBITRATION_LOST;
                            goto stop;
                        }
                    }
                }
            }
            else // send active symbol
            {
                delay = (temp_byte & 0x80) ? TX_SHORT : TX_LONG; // send correct pulse lenght
                __txPin.write(ACTIVE);                           // set bus active
                now = micros();
                while (micros() - now < delay)
                    ;
            }

            temp_byte <<= 1; // next bit
        }                    // end nbits while loop
        ++msg_buf;           // next byte from buffer
    } while (--nbytes);      // end nbytes do loop

    // EOF
    __txPin.write(PASSIVE);
    now = micros();
    delayMicroseconds(PROPOGATION_DELAY);            // Allow time for RX to follow TX for fast processors
    while (micros() - now < TX_SOF)
    {
        if (__rxPin.read() == ACTIVE)
        {
            result = J1850_ERR_ARBITRATION_LOST;
            goto stop;
        }
    }

stop:
    //resumeInterrupts
	attachInterrupt(digitalPinToInterrupt(__rxPin._pin), &J1850VPWFriend::__handleRnChange, CHANGE);
    return handleErrorsInternal(J1850_Write, result);
}

// NB! Performance critical!!! Do not split
uint8_t J1850VPW::sendWithNoCRC(uint8_t *pData, uint8_t nbytes, int16_t timeoutMs /*= -1*/)
{
    if (isReadonly())
    {
        return handleErrorsInternal(J1850_Write, J1850_ERR_RECV_NOT_CONFIGURATED);
    }

    J1850_ERRORS result = J1850_OK;
    static uint8_t buff[BS];
    memcpy(buff, pData, nbytes);
    //Do not add CRC
	
	/*Serial.print("QQQ: ");
     for (int q = 0; q < nbytes; q++) {
         uint8_t w = buff[q];
         if (w < 0x10) {
             Serial.print('0');
         }
         Serial.print(String(w, HEX));
     }
     Serial.println();*/
    

    uint8_t *msg_buf = buff;
    unsigned long now;

    // wait for idle
    if (timeoutMs >= 0)
    {
        now = micros();
        unsigned long  start = now;
        timeoutMs *= 1000; // convert to microseconds
        while (micros() - now < TX_EOF)
        {
            if (__rxPin.read() == ACTIVE)
            {
                now = micros();
            }

            if (micros() - start > timeoutMs)
            {
                result = J1850_ERR_BUS_IS_BUSY;
                goto stop;
            }
        }
    }

    // SOF
	
    //pauseInterrupts
	detachInterrupt(digitalPinToInterrupt(__rxPin._pin));
    __txPin.write(ACTIVE);
    now = micros();
    while (micros() - now < TX_SOF)
        ;

    // send data
    do
    {
        uint8_t temp_byte = *msg_buf; // store byte temporary
        uint8_t nbits = 8;
        unsigned long  delay;
        while (nbits--) // send 8 bits
        {
            if (nbits & 1) // start allways with passive symbol
            {
                delay = (temp_byte & 0x80) ? TX_LONG : TX_SHORT; // send correct pulse lenght
                __txPin.write(PASSIVE);                          // set bus active
                now = micros();
                delayMicroseconds(PROPOGATION_DELAY);            // Allow time for RX to follow TX for fast processors
                while (micros() - now < delay)
                {
                    if (__rxPin.read() == ACTIVE)
                    {
                        if (delay - (micros() - now)  < RX_ARBITRATION_TOL) //Arbitration not lost
                        {
                            now = micros() - delay - 1; //resync to faster module
                        } else //We lost arbitration so drop out
                        {
                            //result = J1850_ERR_ARBITRATION_LOST;
                            //goto stop;
							now = micros() - delay - 1;
                        }
                    }
                }
            }
            else // send active symbol
            {
                delay = (temp_byte & 0x80) ? TX_SHORT : TX_LONG; // send correct pulse lenght
                __txPin.write(ACTIVE);                           // set bus active
                now = micros();
                while (micros() - now < delay)
                    ;
            }

            temp_byte <<= 1; // next bit
        }                    // end nbits while loop
        ++msg_buf;           // next byte from buffer
    } while (--nbytes);      // end nbytes do loop

    // EOF
    __txPin.write(PASSIVE);
    now = micros();
    delayMicroseconds(PROPOGATION_DELAY);            // Allow time for RX to follow TX for fast processors
    while (micros() - now < TX_SOF)
    {
        if (__rxPin.read() == ACTIVE)
        {
            result = J1850_ERR_ARBITRATION_LOST;
            goto stop;
        }
    }

stop:
    //resumeInterrupts	
	attachInterrupt(digitalPinToInterrupt(__rxPin._pin), &J1850VPWFriend::__handleRnChange, CHANGE);
    return handleErrorsInternal(J1850_Write, result);
}



