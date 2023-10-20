#ifndef RELE_LIB
#define RELE_LIB

#include <Arduino.h>

class Relay
{
    public: 
        Relay(byte pin);
        //byte getpin();
        void on();
        void off();
        
    private:
        byte _pinNum;
};

#endif