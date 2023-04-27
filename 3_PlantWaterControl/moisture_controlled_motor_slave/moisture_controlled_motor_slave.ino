/**
  Name: moisture_controlled_motor_slave
  Purpose: measures the water moisture content in the plant soil and activates a pump when the moisture is below a MOISTURE_SETPOINT value.
  Send the values to the master and receive instructions 
  @author: Enrico Ghidoni
  @version: 1.0 23/04/20
*/

// Libraries =========================================================================================|
#include <SoftwareSerial.h>       // To interface with the LoRa device

// Variables =========================================================================================|
// Pump
const int pumpPin = 9;                // Pin to activate the motor
int pumpState = 0;                // Represent whether the pump is on or off
int activatePump = 0;             // Boolean to activate the pumo from the master 

// Moisture sensor
const int moisturePin = A0;               // Pin for the moisture sensor 
const int maximumMoistureValue = 1023;    // Maximum value measurable by the moisture sensor
const float moistureSetpoint = 800;          // Moisture setpoint to activate the pump
float moistureRead;                       // Reading from the moisture sensor [0-maximumMoistureValue]
float moistureValue;                      // Moisture value normalized from the moisture sensor [%]

// LoRa device
#define LORA_PINTX 3          // Connected to the lora RX (D6 in the ESP, change it if you use arduino) 
#define LORA_PINRX 2          // Connected to the lora TX (D5), was 14 in Marco's sketch
const int FREQ_1 = 863000000;  // Radio frequency for data transmission [Hz] for the 3 blocks
const int FREQ_2 = 864000000;
const int FREQ_3 = 865000000;
const int FREQ_B = 866000000;
const int WATCHDOG = 10000;    // Timer used to detect malfunctions in rx (or tx? or both?)
const int SLEEP_TIME;          // Max sleep time is 4'294'967'296 ms
String str, data_str;          // String
int data, instr;               // data and instuctions for communication LoRa-server
int data_tx;                   // Data to be sent to the master
int data_rx;                   // Data received from the master
SoftwareSerial loraSerial(LORA_PINRX, LORA_PINTX);

// Setup ==============================================================================================|
void setup()
  {
    pinMode(pumpPin, OUTPUT);    // Setup the digital pin to activate the motor
    Serial.begin(9600);           // Initiate the serial output on screen
    setupLora();                  // Setup the LoRa device with the function defined below
    Serial.println("Setup complete ===============================================================|");
    Serial.print("pumpState : ");
    Serial.println(pumpState);
  }

// Loop ===============================================================================================|
void loop()
  {
    // Plant moisture control==========================================================================|
    // Logic: read the moisture and the value from the server and activate the pump accordingly
    moistureRead = analogRead(moisturePin);
    moistureValue = moistureRead/maximumMoistureValue * 100;
    if (moistureRead < moistureSetpoint)
      {
        pumpState = 1;
      }
    else
      {
        pumpState = 0;
      }

    // Output of the block: print on the serial screen and activate or deactivate the pump
    Serial.println("\n|================================================================================|");
    Serial.println("SOIL MOISTURE CONTROL");
    Serial.print("moistureRead < moistureSetpoint: ");
    Serial.println((moistureRead < moistureSetpoint));
//    Serial.print("Moisture setpoint: ");
//    Serial.println(moistureSetpoint);
//    Serial.print("Moisture read: ");
//    Serial.println(moistureRead);
    Serial.print("Moisture level: ");
    Serial.print(moistureValue);
    Serial.println("%");
    Serial.print("Pump state: ");
    if (pumpState == 1)
      {
        Serial.println("ON");
        digitalWrite(pumpPin, HIGH);
      }
    else
      {
        Serial.println("OFF");
        digitalWrite(pumpPin, LOW);
      }
    Serial.print("The pumpPin is set to: ");
    Serial.println(digitalRead(pumpPin));
    delay(3000);
    
    Serial.println("~~~~~~~~~~");

    // LoRa loop - Reception and transmission
    /*
      Coherently with the scheme drawn by Marco.
      1. The LoRa module receives the sync message, using the receiveSync function and wait for the necessary time (for what?).
      2. Switch to the frequency of the block.
      3. Send the data to the master and check it is sent correctly.
      4. Receive instructions from the master.
      5. Execute the instructions.
      6. Set the LoRa module in sleep mode until the next iteration.
    */

  /*  
    // 1. Wait for the sync message
    Serial.println("COMMUNICATION WITH THE MASTER");
    Serial.println("Waiting for the sync message...");
    receiveSync();
    delay(4000);

    // 2. Switch to freq 3
    loraSerial.print("radio set freq ");
    loraSerial.println(FREQ_3);
    str = loraSerial.readStringUntil('\n');
    
    // 3. Send data to the master and check it is sent correctly
    Serial.println("Sending data to the master...");
    data_tx = moistureValue;
    loraSerial.print("radio tx ");
    loraSerial.println(data_tx);  // *** PUT HERE THE DATA TO SEND TO THE MASTER ***
    // ADD pumpState
    checkTransmission();

    // 4. Receive instructions from master block
    Serial.println("Waiting for instructions from the master...");
    data_rx = receiveData();
    Serial.print("Data received from the master: ");
    Serial.println(data_rx);

    // 5. Execute the instructions
    delay(1000);
*/
    // 6. Set the lora module in sleep mode untile the next iteration
    loraSerial.print("sys sleep ");     // Puts the system to speed for the specified number of ms (SLEEP_TIME)
    loraSerial.println(SLEEP_TIME);
    str = loraSerial.readStringUntil('\n');

  }

// ============================================================================================|
// Functions used in the setup and loop
/** Function ==================================================================================|
  function name: setupLora
  Setup the LoRa component for data exchange with the master.
*/
void setupLora()
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
    loraSerial.println(FREQ_B);
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

// ============================================================================================|

/** Function ==================================================================================|
  function name: receiveSync
  Function used to sync the slave with the master, similar to receiveData but without saving the data.
*/
//very similar to receiveData, but it does not save the data
//change the 1 after radio_rx if you change the sync message

void receiveSync()
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
        else
          {
            Serial.println("└── Sync message not received.");
          }
      }
    else if ( str.indexOf("invalid_param") == 0)
      {
        Serial.println("└── Parameter not valid");
      }
    else if ( str.indexOf("radio_err") == 0)
      {
        Serial.println("└── Reception not succesful, reception time out occured");
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
void checkTransmission()
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
            Serial.println("└── Transmission failed unexpectedly.");
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

int receiveData()
  {
    int data=0;
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
            Serial.println("data received");
            int space_index = str.indexOf(' ');
            data_str=str.substring(space_index+1); //keeps only the <data> part of the string
            data= data_str.toInt(); //trnasforms the string into an int
          }
        else if ( str.indexOf("radio_err") == 0 ) 
          {
            Serial.println("└── Reception not succesful, reception time-out occurred.");
          }
        else
          {
            Serial.println("└── Reception not succesful; for unknown reasons.");
          }
      }
      else
      {
        Serial.println("└── Reception failed without any expected response. Is the transceiver well connected?");
      }
      return data;
  }