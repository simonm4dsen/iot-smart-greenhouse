/**
  Name: moisture_controlled_motor_slave
  Purpose: measures the water moisture content in the plant soil and activates a pump when the moisture is below a MOISTURE_SETPOINT value
  
  @author: Enrico Ghidoni
  @version: 1.0 23/04/20
*/

#include <SoftwareSerial.h>

#define moisturePin A0    // Analog pin for moisture sensor
#define LORA_PINTX 12     // Connected to the lora RX (D6 in the ESP, change it if you use arduino) 
#define LORA_PINRX 14     // Connected to the lora TX (D5)
#define FREQ_1 863000000  // Radio frequency for data transmission [Hz]
#define FREQ_2 864000000
#define FREQ_3 865000000
#define FREQ_B 866000000
#define WATCHDOG 10000    // Timer used to detect malfunctions in rx (or tx?)
#define SLEEP_TIME

SoftwareSerial loraSerial(LORA_PINRX, LORA_PINTX);

// Variables ===================================================================================|
// Motor
const int MOTOR_PIN = 2;
int motorState = 0;

// Soil moisture sensor
const int MOISTURE_SETPOINT = 99;
float moistureValue = 0;
float moistureMaximum = 1023.00;

// LoRa module
String str, data_str;
int data, instr;


// Setup =======================================================================================|
void setup() 
  {
  pinMode(MOTOR_PIN, OUTPUT); 
  Serial.begin(9600); // Serial communication to PC
  setupLora();
  Serial.println("Setup completed =========================================="); 
  }


// Loop ========================================================================================|
void loop()
{
  // Block 3 - Plant water control
  moistureValue = analogRead(moisturePin)/moistureMaximum*100; 
  if (moistureValue < MOISTURE_SETPOINT) {
    digitalWrite(MOTOR_PIN, HIGH);
    motorState = 1;
  } else {
    digitalWrite(MOTOR_PIN, LOW);
    motorState = 0;
  }
  Serial.print("Soil moisture: ");
  Serial.print(moistureValue);
  Serial.println("%");
  Serial.print("Motor state: ");
  Serial.println(motorState);
  delay(1000); 

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

  // 1. Wait for the sync message
  Serial.println("waiting for sync message");
  receiveSync();
  delay(4000);

  // 2. Switch to freq 3
  loraSerial.print("radio set freq: ");
  loraSerial.println(FREQ_3);
  str = loraSerial.readStringUntil('\n');
  
  // 3. Send data to the master and check it is sent correctly
  Serial.println("Sending data to the master.");
  loraSerial.print("radio tx ");
  loraSerial.println("999");  // *** PUT HERE THE DATA TO SEND TO THE MASTER ***
  checkTransmission();

  // 4. Receive instructions from master block
  Serial.println("Waiting for instructions from the master.");
  data=receiveData();
  Serial.println(data);
  
  // 5. Execute the instructions

  // ... perform the actions ...
  delay(1000);

  // 6. Set the lora module in sleep mode untile the next iteration
  loraSerial.print("sys sleep "); //max sleep time is 4'294'967'296 ms
  loraSerial.println(SLEEP_TIME);
  str = loraSerial.readStringUntil('\n');
}


// ============================================================================================|
// Functions used in the setup and loop


/** Function ==================================================================================|
  function name: setupLora

  Setup the LoRa component for data exchange with the master.
  
  @param
  @return
*/

void setupLora() {

  loraSerial.begin(9600);           // Serial communication to RN2483
  loraSerial.setTimeout(1000);

  loraSerial.listen();
  
  loraSerial.println("sys get ver");
  str = loraSerial.readStringUntil('\n');
  Serial.println(str);
  
  loraSerial.println("mac pause");  // Necessary before radio commands
  str = loraSerial.readStringUntil('\n');
  Serial.println(str);  

  loraSerial.println("radio set mod lora");
  str = loraSerial.readStringUntil('\n');
  Serial.println(str);

  loraSerial.print("radio set freq ");
  loraSerial.println(FREQ_B);
  str = loraSerial.readStringUntil('\n');
  Serial.println(str);

  loraSerial.println("radio set pwr 3");  //max power 14 dBm,  -3 is the min, 3 good for power saving
  str = loraSerial.readStringUntil('\n');
  Serial.println(str);

  loraSerial.println("radio set sf sf7"); //min range, fast data rate, minimum battery impact
  str = loraSerial.readStringUntil('\n');
  Serial.println(str);
  
  loraSerial.println("radio set afcbw 41.7"); //sets the value used by the automatic frequency correction bandwidth
  str = loraSerial.readStringUntil('\n');
  Serial.println(str);
  
  loraSerial.println("radio set rxbw 125"); //Lower receiver BW equals better link budget / SNR (less noise)
  str = loraSerial.readStringUntil('\n');
  Serial.println(str);

  loraSerial.println("radio set prlen 8"); //sets the preamble lenght
  str = loraSerial.readStringUntil('\n');
  Serial.println(str);
  
  loraSerial.println("radio set crc on"); //able to correct single-bit errors and detect many multiple-bit errors
  str = loraSerial.readStringUntil('\n');
  Serial.println(str);
  
  loraSerial.println("radio set iqi off");
  str = loraSerial.readStringUntil('\n');
  Serial.println(str);
  
  loraSerial.println("radio set cr 4/5"); //every 4 useful bits are going to be encoded by 5, transmission bits
  str = loraSerial.readStringUntil('\n');
  Serial.println(str);
  
  loraSerial.print("radio set wdt "); //set the whatch dog timer,  disable for continuous reception
  loraSerial.println(WATCHDOG);
  str = loraSerial.readStringUntil('\n');
  Serial.println(str);
  
  loraSerial.println("radio set sync 12"); //set the sync word used
  str = loraSerial.readStringUntil('\n');
  Serial.println(str);
  
  //For a fixed SF, a narrower bandwidth will increase sensitivity as the bit rate is reduced
  //this means more time-on-air, so more battery consumption, but it's easier to receive
  loraSerial.println("radio set bw 125");
  str = loraSerial.readStringUntil('\n');
  Serial.println(str);
}

// ============================================================================================|

/** Function ==================================================================================|
  function name: receiveSync

  Function similar to receiveData, used to sync the slave with the master.
  

  @param
  @return
*/

//very similar to receiveData, but it does not save the data
//change the 1 after radio_rx if you change the sync message

void receiveSync()
  {
    loraSerial.println("radio rx 0"); // Wait to receive until the watchdog time
    str = loraSerial.readStringUntil('\n');
    delay(20);
    if ( str.indexOf("ok") == 0 ) //check if the parameters are correct, and we are in rx mode
      {
        str = String("");
        while(str=="")
          {
            str = loraSerial.readStringUntil('\n');
          }
        if ( str.indexOf("radio_rx 1") == 0 )  //checking if the sync message is received 
          {
            Serial.println("sync message received");
          }
        else
          {
            Serial.println("Received nothing");
          }
      }
    else
    {
      Serial.println("radio not going into receive mode");
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
  
  @param colorCode color code to convert.
  @return the numerical value of the color code.
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
            Serial.println("Message sent correctly."); //printing received data
          }
        else
          {
            Serial.println("Transmission failed.");
          }
      }
    else
      {
        Serial.println("Transmission failed.");
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
  
  @param colorCode color code to convert.
  @return the numerical value of the color code.
*/

int receiveData(){
  int data=0;
  loraSerial.println("radio rx 0"); //wait for to receive until the watchdogtime
  str = loraSerial.readStringUntil('\n');
  delay(20);
  if ( str.indexOf("ok") == 0 ) //check if the parameters are correct, and we are in rx mode
  {
    str = String("");
    while(str=="")
    {
      str = loraSerial.readStringUntil('\n');
    }
    if ( str.indexOf("radio_rx") == 0 )  //checking if data was received (equals radio_rx = <data>)
    {
      Serial.println("data received");
      int index = str.indexOf(' ');
      data_str=str.substring(index+1); //keeps only the <data> part of the string
      data= data_str.toInt(); //trnasforms the string into an int
    }
    else
    {
      Serial.println("Received nothing");
    }
  }
  else
  {
    Serial.println("radio not going into receive mode");
  }
  return data;
}
