/*
  SlaveToMaster.h - Library for communication between slave blocks and master block.
  Created by Enrico Ghidoni, April 27, 2023 for the DTU course
  34346 Networking technologies and application development for Internet of Things (IoT).
*/

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

/** Function ==================================================================================|
  function name: setupLora
  Setup the LoRa component for data exchange with the master.
*/

void SlaveToMaster::setupLora()
  {
    loraSerial.begin(9600);                     // Serial communication to RN2483
    loraSerial.setTimeout(1000);                // Sets the maximum time (ms) to wait for serial data. Default = 1000 ms

    loraSerial.listen();                        // Enables the selected SoftwareSerial object to listen
    
    loraSerial.println("sys get ver");          // Returns information on hardware platform, firmware version and release date
                                                // From Marco: prints data to the transmit pin of the SoftwareSerial object followed by a carriage return and line feed
    str = loraSerial.readStringUntil('\n');     // Reads characters from the serial buffer into the String str up until a terminator (newline in this case). 
    Serial.println(str);                        // Print data to the serial port
    
    loraSerial.println("mac pause");            // Pauses LoRaWAN stack functionality to allow transceiver (radio) configuration. Necessary before radio commands
    str = loraSerial.readStringUntil('\n');
    Serial.println(str);  

    loraSerial.println("radio set mod lora");   // Set the module Modulation mode to LoRa
    str = loraSerial.readStringUntil('\n');   
    Serial.println(str);

    loraSerial.print("radio set freq ");        // Set the current operation frequency for the radio to FREQ_B
    loraSerial.println(freq);
    str = loraSerial.readStringUntil('\n');
    Serial.println(str);

    loraSerial.println("radio set pwr 3");      // Set the output power level used by the radio during tx to max power 14 dBm. -3 is the min, 3 good for power saving
    str = loraSerial.readStringUntil('\n');
    Serial.println(str);

    loraSerial.println("radio set sf sf7");     //Set the requested spreading factor (SF); min range, fast data rate, minimum battery impact
    str = loraSerial.readStringUntil('\n');
    Serial.println(str);
    
    loraSerial.println("radio set afcbw 41.7"); // Sets the value used by the automatic frequency correction bandwidth
    str = loraSerial.readStringUntil('\n');
    Serial.println(str);
    
    loraSerial.println("radio set rxbw 125");   // Set the operational receive bandwith: lower receiver bw equals better link budget / signal noise ratio (→ less noise)
    str = loraSerial.readStringUntil('\n');
    Serial.println(str);

    loraSerial.println("radio set prlen 8");    // Sets the preamble length used during tx
    str = loraSerial.readStringUntil('\n');
    Serial.println(str);
    
    loraSerial.println("radio set crc on");     // Set if a cycle redundancy check (CRC) is to be used; able to correct single-bit errors and detect many multiple-bit errors
    str = loraSerial.readStringUntil('\n');
    Serial.println(str);
    
    loraSerial.println("radio set iqi off");    // Set if IQ inversion is to be used
    str = loraSerial.readStringUntil('\n');
    Serial.println(str);
    
    loraSerial.println("radio set cr 4/5");     // Set the coding rate used by the radio: every 4 useful bits are going to be encoded by 5 transmission bits
    str = loraSerial.readStringUntil('\n');
    Serial.println(str);
    
    loraSerial.print("radio set wdt ");         // Set the time-out limit for the whatch dog timer, disable (set to 0) for continuous reception
    loraSerial.println(WATCHDOG);
    str = loraSerial.readStringUntil('\n');
    Serial.println(str);
    
    loraSerial.println("radio set sync 12");    // Set the sync word used
    str = loraSerial.readStringUntil('\n');
    Serial.println(str);
    
    // For a fixed SF, a narrower bandwidth will increase sensitivity as the bit rate is reduced;
    // this means more time-on-air, so more battery consumption, but it's easier to receive
    loraSerial.println("radio set bw 125");     // Set the value used for the radio bandwith  (can be 125, 250, 500)
    str = loraSerial.readStringUntil('\n');
    Serial.println(str);
  }


/** Function ==================================================================================|
  function name: receiveSync
  Function used to sync the slave with the master, similar to receiveData but without saving the data.
*/
//very similar to receiveData, but it does not save the data
//change the 1 after radio_rx if you change the sync message

void SlaveToMaster::receiveSync()
  {
    loraSerial.println("radio rx 0");                  // Set the receiver in Continuous Reception mode
    str = loraSerial.readStringUntil('\n');
    delay(20);
    if ( str.indexOf("ok") == 0 )                      // Check if the parameters are correct, and we are in rx mode
      {
        str = String("");
        while(str=="")
          {
            str = loraSerial.readStringUntil('\n');
          }
        if ( str.indexOf("radio_rx 1") == 0 )          // Receive the response from the master: if 1 all good 
          {
            Serial.println("└── Sync message received.");
          }
        else if ( str.indexOf("radio_err") == 0 )      //  
          {
            Serial.println("└── Reception was not succesful, reception time has occurred.");
          }
        else
          {
            Serial.println("└── Sync message not received, without any expected response. Is the transceiver well connected?");
          }
      }
    else if ( str.indexOf("invalid_param") == 0)
      {
        Serial.println("└── Parameter not valid");
      }
    else if ( str.indexOf("busy") == 0)
      {
        Serial.println("└── Reception not succesful, the transceiver is currently busy.");
      }
    else
      {
        Serial.println("└── Radio not going into Reception mode without any expected response. Is the transceiver well connected?");
      }
  }

// ============================================================================================|

/** Function
  function name: checkTransmission
  Prints in the serial monitor "message sent correctly" if there are no errors, or "transmission failed"
  After sending "radio tx <data>", the module prints two messages.
    1. "ok" if the parameters are correct and the module is in tx mode, OR something like "invalid param" if there is a problem.
       Check the RN2483 user manual on DTU Learn, week 5 for details on the exact message.
    2. "radio_tx_ok" if the transmission was successfull.
*/
void SlaveToMaster::checkTransmission()
  {
    str = loraSerial.readStringUntil('\n');
    delay(20);
    if ( str.indexOf("ok") == 0 ) //check if the parameters are correct, and we are in tx mode
      {
        str = String("");
        while(str=="")
          {
            str = loraSerial.readStringUntil('\n');
          }
        if (str.indexOf("radio_tx_ok") == 0)  //checking if data was sent correctly
          {
            Serial.println("└── Message sent correctly."); //printing received data
          }
        else if (str.indexOf("radio_err") == 0)
          {
            Serial.println("└── Transmission interrupted by radio Watchdog Timer time-out.");
          }
        else
          {
            Serial.println("└── Transmission failed without any expected response.");
          }
      }
    else if (str.indexOf("invalid_param") == 0)
      {
        Serial.println("└── Parameter not valid.");
      }
    else if (str.indexOf("busy") == 0)
      {
        Serial.println("└── Transceiver currently busy.");
      }
    else
      {
        Serial.println("└── Transmission failed without any expected response. Is the transceiver well connected?");
      }
  }

// ============================================================================================|

/** Function
  function name: receiveData
  After we use the command "radio rx 0", the module goes in continous reception mode until it receives something or untile the whatchdog timer expires
  After the command, the module prints two messages.
  the first one is "ok" if the parameters are correct and the module is in rx mode, or something like "invalid param" if there is a problem. 
  check the RN2483 user manual on dtu Learn, week 5 if you need the exact message
  the second one is "radio_rx <data>" if the reception was successfull
  this function takes the data received, it prints it in the serial monitor and it saves it as an int in the variable "data"
*/

int SlaveToMaster::receiveData()
  {
    int data_rx=0;
    loraSerial.println("radio rx 0");               // Set the receiver in Continuous Reception mode
    str = loraSerial.readStringUntil('\n');
    delay(20);
    if ( str.indexOf("ok") == 0 )                   // Parameter is valid and the transceiver is congifured in Receive mode
      {
        str = String("");
        while(str=="")
          {
            str = loraSerial.readStringUntil('\n');
          }
        if ( str.indexOf("radio_rx") == 0 )         // Check if reception was succesful, meaning radio_rx <data>
          {
            Serial.println("└── Data received");
            int space_index = str.indexOf(' ');
            data_str = str.substring(space_index + 1); //keeps only the <data> part of the string
            data_rx = data_str.toInt(); //trnasforms the string into an int
          }
        else if ( str.indexOf("radio_err") == 0 ) 
          {
            Serial.println("└── Reception not succesful, reception time-out occurred.");
          }
        else
          {
            Serial.println("└── Reception not succesful without an expected response.");
          }
      }
    else if ( str.indexOf ("invalid_param") == 0 )
      {
        Serial.println("└── Reception failed because parameter is not valid");
      }
    else if (str.indexOf ("busy") == 0)
      {
        Serial.println("└── Reception failed because the transceiver is busy.");
      }
    else
      {
        Serial.println("└── Reception failed without any expected response. Is the transceiver well connected?");
      }
    return data;
  }
