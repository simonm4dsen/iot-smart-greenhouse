#include "Arduino.h"
#include "SlaveToMaster.h"

SlaveToMaster::SlaveToMaster(SoftwareSerial loraSerial)
{
  _loraSerial = loraSerial;
}

void SlaveToMaster::begin()
{
    setupLora(loraSerial);      // Setup the LoRa device with the function defined below
}