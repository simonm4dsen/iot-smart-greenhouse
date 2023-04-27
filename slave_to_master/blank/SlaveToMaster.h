/*
  SlaveToMaster.h - Library for communication between slave blocks and master block.
  Created by Enrico Ghidoni, April 27, 2023 for the DTU course
  34346 Networking technologies and application development for Internet of Things (IoT).
*/

#ifndef SlaveToMaster_h
#define SlaveToMaster_h

#include "Arduino.h"
#include <SoftwareSerial.h>

class SlaveToMaster
{
  public:
    SlaveToMaster();
    void setupLora(SoftwareSerial loraSerial);
    void receiveSync(SoftwareSerial loraSerial);
    void checkTransmission(SoftwareSerial loraSerial);
    void receiveData(SoftwareSerial loraSerial);
    SoftwareSerial _loraSerial;
  private:
};

#endif
