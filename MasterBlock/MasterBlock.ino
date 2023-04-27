// imports
#include <ESP8266WiFi.h>
#include "src/iotc/common/string_buffer.h"
#include "src/iotc/iotc.h"
#include <ArduinoJson.h> //Library for handling/unpacking JSON format

#include <SoftwareSerial.h>

// Define variables and pinouts

//#define FAN_LED_PIN D1
//#define PUMP_LED_PIN D2
#define FAN_ON_TIME 10000 // 10 seconds
#define PUMP_ON_TIME 15000 // 15 seconds

// Block parameters
const unsigned long TELEMETRY_INTERVAL = 5000;

float temperatureThreshold = 30.0;
float soilMoistureThreshold = 50.0;

unsigned long fanOffTime = 0;
unsigned long pumpOffTime = 0;

// Azure IoT hub connection strings
const char* SCOPE_ID = "0ne009E225E";
const char* DEVICE_ID = "1hihltp023e";
const char* DEVICE_KEY = "+9VWNWUEuL6C6XeNDI9jkw4+b8S8Fa/vpuSr54qvOp8=";

#define WIFI_SSID "Simon - iPhone"
#define WIFI_PASSWORD "IoT11223344"

//these pins are thinked for the ESP, since the professor suggested that it would be the best option
//also, when setting the timings, remember to check the duty cicle restrictions
// Tansmitter (White)
#define LORAT_PINTX 12 //connected to the lora RX (D6)
#define LORAT_PINRX 14 //connected to the lora TX (D5)
// Reciever (Dark)
#define LORAR_PINTX 4 //connected to the lora RX (D2)
#define LORAR_PINRX 5 //connected to the lora TX (D1)

#define FREQ_1 863000000
#define FREQ_2 864000000
#define FREQ_3 865000000

#define FREQ_B 866000000 // Broadcast frequency - All slaves recieves this, and then swaps to own frequency
#define sync_msg 1

#define WATCHDOG 10000 //this watchdog set for how much time the lora module waits for a message

SoftwareSerial loraTxSerial(LORAT_PINRX, LORAT_PINTX);
SoftwareSerial loraRxSerial(LORAR_PINRX, LORAR_PINTX);
/*I implemented the two lora modules using software serial, as i said this should not be a problem for the p2p communication, but maybe it could be one with the loraWAN
because the system can listen to only one module at a time, and the listen also apparently cancels the buffers of the messages in the modules
*/

String str, data_str;
int data1, data2, data3;

//When recieved command -> action
void on_event(IOTContext ctx, IOTCallbackInfo* callbackInfo);
#include "src/connection.h"

void setup() {
  Serial.println("setup Lora");
  Serial.begin(57600);  // Serial communication to PC
  setupLoraRx();
  setupLoraTx();

  wifi_setup();

  Serial.println("setup completed");
}

void loop() {
  // communication with the modules
/* this part was here because my idea was that every time the loop starts the matser talks with the other blocks, than it talks to the server untile the end of the loop
  as a consequence when the loop starts again there is the need to set the modules to use Lora P2P again  

  setupLora(loraTxSerial); //default frequency is broadcast
  setupLora(loraRxSerial); //default is freq2, because it's the one i wanted to use for testing since it has both transmission and reception
*/  

  //send the syncronization message and check it is sent correctly
  loraTxSerial.print("radio set freq ");
  loraTxSerial.println(FREQ_B);
  str = loraTxSerial.readStringUntil('\n');  

  Serial.println("sending sync message");
  loraTxSerial.print("radio tx ");
  loraTxSerial.println(sync_msg);
  checkTransmission();
  delay(1000);

  // --------------- receive data from block 1 - TEMP/HUMIDITY BLOCK (OBS: This was Block 2 in the original sketch)
  //RX switch to freq 1
  //loraRxSerial.print("radio set freq ");
  //loraRxSerial.println(FREQ_1);
  //str = loraRxSerial.readStringUntil('\n');
  
  //Serial.println("waiting for a message from block 1");
  //data1=receiveData();

  //delay(4000);  
  //I put some random delays in order for the master to wait for the other blocks (in the moments when it should do it, as in the scheme I sent on teams). 
  //if the master needs to do anything esle in that period of time you can save the current time with millis, then check with a loop when you go past this time+the waiting time


  // --------------- receive data from block 2 - FAN BLOCK (OBS: This was block 1 in the original Sketch)
  //RX switch to freq 2
  Serial.println("RX to FREQ 2");
  loraRxSerial.print("radio set freq ");
  loraRxSerial.println(FREQ_2);
  str = loraRxSerial.readStringUntil('\n');
  
  Serial.println("waiting for a message from block 2");
  data2=receiveData();
  Serial.println(data2);
  
  //compute instructions for the fan using data1 and data2
  //when you define how you want to command the fan put here the equations, and then put the instructions instead of the 111 after radio tx at line 81 

  //TX switch to freq 2
  loraTxSerial.print("radio set freq ");
  loraTxSerial.println(FREQ_2);
  str = loraTxSerial.readStringUntil('\n');  

  //send the fan instructions and check it is sent correctly
  Serial.println("sending fan instructions");
  loraTxSerial.print("radio tx ");
  loraTxSerial.println("111");
  checkTransmission();

  delay(10000);

  // --------------- receive data from block 3
  //RX switch to freq 3
  //loraRxSerial.print("radio set freq ");
  //loraTxSerial.println(FREQ_3);
  //str = loraRxSerial.readStringUntil('\n');
  
  // --------------- receive data from block 3
  //Serial.println("waiting for a message from block 3");
  //data3=receiveData();

  //compute instructions for the pump using data1, data2 and data3
  //same thing as the fan instructions

  //TX switch to freq 3
  //loraTxSerial.print("radio set freq ");
  //loraTxSerial.println(FREQ_3);
  //str = loraTxSerial.readStringUntil('\n');  

  //send the pump instructions and check it is sent correctly
  //Serial.println("sending pump instructions");
  //loraTxSerial.print("radio tx ");
  //loraTxSerial.println("------");
  //checkTransmission();
  
  //delay(-------)
  
  
  //comunication with Cloud
  // Generate random values
  float humidity = random(2000, 8000) / 100.0;
  float temperature = random(0, 50);
  float SoilMoisture = random(0, 10000) / 100.0;
  int LightLevel = random(0, 1000);

  if (!isConnected) {
    // Re-establish connection // UART Interrupt might be appropriate
    client_reconnect();
  }

  if (isConnected) {
    unsigned long ms = millis();
    if (ms - lastTick > 10000) {  // send telemetry every 10 seconds
      char msg[128] = {0};
      int pos = 0, errorCode = 0;

      lastTick = ms;

      // Send telemetry data
      pos = snprintf(msg, sizeof(msg) - 1, "{\"temperature\": %f, \"humidity\": %f, \"SoilMoisture\": %f, \"LightLevel\": %d}", temperature, humidity, SoilMoisture, LightLevel);
      
      errorCode = iotc_send_telemetry(context, msg, pos);

      if (errorCode != 0) {
        LOG_ERROR("Failed to send telemetry. errorCode=%d", errorCode);
      } else {
        LOG_VERBOSE("Telemetry sent");
      }
        // Debugging
        Serial.print("Temperature threshold: ");
        Serial.println(temperatureThreshold);
        Serial.print("Soil moisture threshold: ");
        Serial.println(soilMoistureThreshold);
    }
  }

  iotc_do_work(context);
 

};


// ----- Helper functions -----

void wifi_setup () {
  Serial.println("setup wifi-connection");
  connect_wifi(WIFI_SSID, WIFI_PASSWORD);
  connect_client(SCOPE_ID, DEVICE_ID, DEVICE_KEY);

  if (context != NULL) {
    lastTick = 0;
  }
}

void client_reconnect () {
  Serial.println("setup client-connection");
  connect_client(SCOPE_ID, DEVICE_ID, DEVICE_KEY);

  if (context != NULL) {
    lastTick = 0;
  }
}

void on_event(IOTContext ctx, IOTCallbackInfo* callbackInfo) {
  // ConnectionStatus
  if (strcmp(callbackInfo->eventName, "ConnectionStatus") == 0) {
    LOG_VERBOSE("Is connected ? %s (%d)",
                callbackInfo->statusCode == IOTC_CONNECTION_OK ? "YES" : "NO",
                callbackInfo->statusCode);
    isConnected = callbackInfo->statusCode == IOTC_CONNECTION_OK;
    return;
  }

  AzureIOT::StringBuffer buffer;
  if (callbackInfo->payloadLength > 0) {
    buffer.initialize(callbackInfo->payload, callbackInfo->payloadLength);
  }

  LOG_VERBOSE("- [%s] event was received. Payload => %s\n",
              callbackInfo->eventName, buffer.getLength() ? *buffer : "EMPTY");

  if (strcmp(callbackInfo->eventName, "Command") == 0) {
    LOG_VERBOSE("- Command name was => %s\r\n", callbackInfo->tag);

    if (strcmp(callbackInfo->tag, "ActivateFan") == 0) {

      // ----- Activate Fan Downlink

      //digitalWrite(FAN_LED_PIN, HIGH);
      //delay(FAN_ON_TIME);
      //digitalWrite(FAN_LED_PIN, LOW);

    } else if (strcmp(callbackInfo->tag, "ActivatePump") == 0) {

      // Activate Pump Downlink

      //digitalWrite(PUMP_LED_PIN, HIGH);
      //delay(PUMP_ON_TIME);
      //digitalWrite(PUMP_LED_PIN, LOW);

    } else if (strcmp(callbackInfo->tag, "TemperatureThreshold") == 0) {

      // Update Temperature Threshold

      float newThreshold = atof(*buffer);
      if (newThreshold >= 0) {
        temperatureThreshold = newThreshold;
        LOG_VERBOSE("Temperature threshold updated to %f", temperatureThreshold);
      }
    } else if (strcmp(callbackInfo->tag, "SoilMoistureThreshold") == 0) {

      // Update Moisture Threshold

      float newThreshold = atof(*buffer);
      if (newThreshold >= 0) {
        soilMoistureThreshold = newThreshold;
        LOG_VERBOSE("Soil moisture threshold updated to %f", soilMoistureThreshold);
      }
    }
  } else if (strcmp(callbackInfo->eventName, "Properties") == 0) {
    // Update device properties
    AzureIOT::StringBuffer propertyBuffer;
    propertyBuffer.initialize(callbackInfo->payload, callbackInfo->payloadLength);

    DynamicJsonDocument jsonDoc(256);
    DeserializationError error = deserializeJson(jsonDoc, *propertyBuffer);
    if (!error) {
      JsonObject jsonObj = jsonDoc.as<JsonObject>();
      if (jsonObj.containsKey("TemperatureThreshold")) {
        temperatureThreshold = jsonObj["TemperatureThreshold"];
      }
      if (jsonObj.containsKey("SoilMoistureThreshold")) {
        soilMoistureThreshold = jsonObj["SoilMoistureThreshold"];
      }
    } else {
      Serial.print("deserializeJson() failed: ");
      Serial.println(error.c_str());
    }
  }
}

/*
After sending something using the command "radio tx <data>", the module prints two messages.
the first one is "ok" if the parameters are correct and the module is in tx mode, or something like "invalid param" if there is a problem. 
check the RN2483 user manual on dtu Learn, week 5 if you need the exact message
the second one is "radio_tx_ok" if the transmission was successfull
this function prints in the serial monitor "message sent correctly" if there are no errors, or "transmission failed"
*/
void checkTransmission() {
  str = loraTxSerial.readStringUntil('\n'); //read the first message
  delay(30);
  if ( str.indexOf("ok") == 0 ) //check if the parameters are correct, and we are in tx mode
  {
    str = String("");
    while(str=="")
    {
      str = loraTxSerial.readStringUntil('\n'); //keep checking until we receive the second message
    }
    if ( str.indexOf("radio_tx_ok") == 0 )  //checking if data was sent correctly
    {
      Serial.println("message sent correctly");
    }
    else
    {
      Serial.println("transmission failed");
    }
  }
  else
  {
    Serial.println("transmission failed");
  }
}


/*
After we use the command "radio rx 0", the module goes in continous reception mode until it receives something or untile the whatchdog timer expires
After the command, the module prints two messages.
the first one is "ok" if the parameters are correct and the module is in rx mode, or something like "invalid param" if there is a problem. 
check the RN2483 user manual on dtu Learn, week 5 if you need the exact message
the second one is "radio_tx <data>" if the transmission was successfull
this function takes the data received, it prints it in the serial monitor and it saves it as an int in the variable "data"
*/

int receiveData(){
  int data=0;
  loraRxSerial.println("radio rx 0"); //wait to receive until the watchdogtime (continous reception)
  str = loraRxSerial.readStringUntil('\n');
  delay(20);
  if ( str.indexOf("ok") == 0 ) //check if the parameters are correct, and we are in rx mode
  {
    str = String("");
    while(str=="")
    {
      str = loraRxSerial.readStringUntil('\n');
    }
    if ( str.indexOf("radio_rx") == 0 )  //checking if data was received (equals radio_rx = <data>)
    {
      Serial.println("data received");
      int index = str.indexOf(' '); //we save the index of the space, so that later we will cut the string and keep only what's after the space
      data_str=str.substring(index+1); //keeps only the <data> part of the string
      data= data_str.toInt(); //transforms the string into an int (i supposed it would be the data type we would need)
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



void setupLoraTx() {
  Serial.println("\nInitiating LoRaTx");
  loraTxSerial.begin(57600);  // Serial communication to RN2483 // 9600
  loraTxSerial.setTimeout(5000);

  loraTxSerial.listen();
  
  loraTxSerial.println("sys get ver");
  str = loraTxSerial.readStringUntil('\n');
  Serial.println(str);
  
  loraTxSerial.println("mac pause"); //necessary before radio commands
  str = loraTxSerial.readStringUntil('\n');
  Serial.println(str);  

  loraTxSerial.println("radio set mod lora");
  str = loraTxSerial.readStringUntil('\n');
  Serial.println(str);

  loraTxSerial.print("radio set freq ");
  loraTxSerial.println(FREQ_B); 
  str = loraTxSerial.readStringUntil('\n');
  Serial.println(str);

  loraTxSerial.println("radio set pwr 3");  //max power 14 dBm, -3 is the min, 3 good for power saving
  str = loraTxSerial.readStringUntil('\n');
  Serial.println(str);

  loraTxSerial.println("radio set sf sf7"); //min range, fast data rate, minimum battery impact
  str = loraTxSerial.readStringUntil('\n');
  Serial.println(str);
  
  loraTxSerial.println("radio set afcbw 41.7"); //sets the value used by the automatic frequency correction bandwidth
  str = loraTxSerial.readStringUntil('\n');
  Serial.println(str);
  
  loraTxSerial.println("radio set rxbw 125"); //Lower receiver BW equals better link budget / SNR (less noise), but htere can be problems of freq drifting
  str = loraTxSerial.readStringUntil('\n');
  Serial.println(str);

  loraTxSerial.println("radio set prlen 8"); //sets the preamble lenght
  str = loraTxSerial.readStringUntil('\n');
  Serial.println(str);
  
  loraTxSerial.println("radio set crc on"); //able to correct single-bit errors and detect many multiple-bit errors
  str = loraTxSerial.readStringUntil('\n');
  Serial.println(str);
  
  loraTxSerial.println("radio set iqi off");
  str = loraTxSerial.readStringUntil('\n');
  Serial.println(str);
  
  loraTxSerial.println("radio set cr 4/5"); //every 4 useful bits are going to be encoded by 5, transmission bits
  str = loraTxSerial.readStringUntil('\n');
  Serial.println(str);
  
  loraTxSerial.print("radio set wdt "); //set the whatch dog timer,  disable for continuous reception
  loraTxSerial.println(WATCHDOG);
  str = loraTxSerial.readStringUntil('\n');
  Serial.println(str);
  
  loraTxSerial.println("radio set sync 321"); //set the sync word used
  str = loraTxSerial.readStringUntil('\n');
  Serial.println(str);
  
  //For a fixed SF, a narrower bandwidth will increase sensitivity as the bit rate is reduced
  //this means more time-on-air, so more battery consumption, but it's easier to receive
  loraTxSerial.println("radio set bw 125");
  str = loraTxSerial.readStringUntil('\n');
  Serial.println(str);
}

void setupLoraRx() {
  Serial.println("\nInitiating LoRaRx");
  loraRxSerial.begin(57600);  // Serial communication to RN2483
  loraRxSerial.setTimeout(1000);

  loraRxSerial.listen();
  
  loraRxSerial.println("sys get ver");
  str = loraRxSerial.readStringUntil('\n');
  Serial.println(str);
  
  loraRxSerial.println("mac pause"); //necessary before radio commands
  str = loraRxSerial.readStringUntil('\n');
  Serial.println(str);  

  loraRxSerial.println("radio set mod lora");
  str = loraRxSerial.readStringUntil('\n');
  Serial.println(str);

  loraRxSerial.print("radio set freq ");
  loraRxSerial.println(FREQ_B);
  str = loraRxSerial.readStringUntil('\n');
  Serial.println(str);

  loraRxSerial.println("radio set pwr 3");  //max power 14 dBm, -3 is the min, 3 good for power saving
  str = loraRxSerial.readStringUntil('\n');
  Serial.println(str);

  loraRxSerial.println("radio set sf sf7"); //min range, fast data rate, minimum battery impact
  str = loraRxSerial.readStringUntil('\n');
  Serial.println(str);
  
  loraRxSerial.println("radio set afcbw 41.7"); //sets the value used by the automatic frequency correction bandwidth
  str = loraRxSerial.readStringUntil('\n');
  Serial.println(str);
  
  loraRxSerial.println("radio set rxbw 125"); //Lower receiver BW equals better link budget / SNR (less noise), but htere can be problems of freq drifting
  str = loraRxSerial.readStringUntil('\n');
  Serial.println(str);

  loraRxSerial.println("radio set prlen 8"); //sets the preamble lenght
  str = loraRxSerial.readStringUntil('\n');
  Serial.println(str);
  
  loraRxSerial.println("radio set crc on"); //able to correct single-bit errors and detect many multiple-bit errors
  str = loraRxSerial.readStringUntil('\n');
  Serial.println(str);
  
  loraRxSerial.println("radio set iqi off");
  str = loraRxSerial.readStringUntil('\n');
  Serial.println(str);
  
  loraRxSerial.println("radio set cr 4/5"); //every 4 useful bits are going to be encoded by 5, transmission bits
  str = loraRxSerial.readStringUntil('\n');
  Serial.println(str);
  
  loraRxSerial.print("radio set wdt "); //set the whatch dog timer,  disable for continuous reception
  loraRxSerial.println(WATCHDOG);
  str = loraRxSerial.readStringUntil('\n');
  Serial.println(str);
  
  loraRxSerial.println("radio set sync 1"); //set the sync word used
  str = loraRxSerial.readStringUntil('\n');
  Serial.println(str);
  
  //For a fixed SF, a narrower bandwidth will increase sensitivity as the bit rate is reduced
  //this means more time-on-air, so more battery consumption, but it's easier to receive
  loraRxSerial.println("radio set bw 125");
  str = loraRxSerial.readStringUntil('\n');
  Serial.println(str);
}

