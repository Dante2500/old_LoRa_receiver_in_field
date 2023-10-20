#include "rele_lib.h"

Relay::Relay(byte pin){
    _pinNum = pin;
    pinMode(_pinNum, OUTPUT); 
}

void Relay::on(){
    digitalWrite(_pinNum, LOW);
}

void Relay::off(){
    digitalWrite(_pinNum, HIGH);
}
