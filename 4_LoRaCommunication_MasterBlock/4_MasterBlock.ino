// imports
#include <ESP8266WiFi.h>
#include "src/iotc/common/string_buffer.h"
#include "src/iotc/iotc.h"
#include <ArduinoJson.h> //Library for parsing JSON format

#include <SoftwareSerial.h>

// Define variables and pinouts
// Block parameters
// Telemetry_interval: Time each block has to complete its operations:
const unsigned long TELEMETRY_INTERVAL = 20000;
// Threshold values
float temperatureThreshold = 30.0;
float soilMoistureThreshold = 50.0;
//Uplink varibales
int activateFan = 0;
int activatePump = 0;
// initialize data variables
float humidity = 0;
float temperature = 0;
float soilMoisture = 0;
int lightLevel = 0;

// Azure IoT hub connection strings
const char* SCOPE_ID = "0ne009E225E";
const char* DEVICE_ID = "1hihltp023e";
const char* DEVICE_KEY = "+9VWNWUEuL6C6XeNDI9jkw4+b8S8Fa/vpuSr54qvOp8=";
//#DEFINE IOTC_CONNECT_CONNECTION_STRING 0x04

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

String str, data_str, temp, data;
String data1, data2, data3;

unsigned long start_time;
unsigned long current_time;
// The entire cycle from first sync-message to data upload to the cloud is fixed
// For the sake of testing the cycle restarts every 1 minute
// Real Scenario to oblige to duty cycle restrictions ~20 minutes
#define CYCLE_TIME 60000

//When recieved command -> action
void on_event(IOTContext ctx, IOTCallbackInfo* callbackInfo);
#include "src/connection.h"

void setup() {
  Serial.println("setup Lora");
  Serial.begin(57600);  // Serial communication to PC
  setupLoraRx();
  setupLoraTx();

  wifi_setup();

  DynamicJsonDocument jsonDoc(256); // For parsing data

  Serial.println("setup completed");
}

void loop() {
  // communication with nodes
  //send the syncronization message and check it is sent correctly
  // This is used to sync the slave-blocks clocks/timings to make sure the Master is ready to recieve data when it attempts a transmission
  Serial.println("Set Tx to Broadcast-FREQ");
  loraTxSerial.print("radio set freq ");
  loraTxSerial.println(FREQ_B);
  str = loraTxSerial.readStringUntil('\n');

  Serial.println("sending sync message");
  loraTxSerial.print("radio tx ");
  loraTxSerial.println(sync_msg);
  checkTransmission();

  // Get base time of when sync meassage was sent
  Serial.println("Start cycle timer");
  start_time = millis();

  // --------------- receive data from block 1 - TEMP/HUMIDITY BLOCK (OBS: This was Block 2 in the original sketch)
  //RX switch to freq 1
  Serial.println("Set Rx to Block 2 FREQ"); // 1
  loraRxSerial.print("radio set freq ");
  loraRxSerial.println(FREQ_1);
  str = loraRxSerial.readStringUntil('\n');
  
  Serial.println("waiting for a message from block 2");
  data1=receiveData();

  DynamicJsonDocument doc(1024);
  deserializeJson(doc, data1);

  // Parse JSON data as individual variables
  float humidity = doc["humidity"];
  float temp = doc["temp"];

  //save the current time with millis, then wait to make sure the master-block goes into reciev-mode when it is expected to
  current_time = millis();
  if (current_time - start_time < TELEMETRY_INTERVAL) { // only sleep if we have time to do so before next cycle is supposed to start
   delay(TELEMETRY_INTERVAL-(current_time - start_time));
 }

  // --------------- receive data from block 2 - FAN BLOCK (OBS: This was block 1 in the original Sketch)
  
  //RX switch to freq 2
  Serial.println("Set Rx to Block 1 FREQ"); //2
  loraRxSerial.print("radio set freq ");
  loraRxSerial.println(FREQ_2);
  str = loraRxSerial.readStringUntil('\n');
  
  Serial.println("waiting for a message from Block 1"); //2
  data2=receiveData();
  deserializeJson(doc, data2); // Parse JSON

  // Parse JSON data as individual variables
  int lightLevel = doc["lightLevel"];
  
  delay(3000); // Make sure we give slave-node time to go into recieve mode

  //TX switch to freq 2
  Serial.println("Set Tx to Block 1 FREQ");
  loraTxSerial.print("radio set freq ");
  loraTxSerial.println(FREQ_2);
  str = loraTxSerial.readStringUntil('\n');  

  //send the fan instructions and check it is sent correctly

  //This part can be optimized by not sending the threshold values at every itteration, but instead only when i updates
  // Also, varible names should have been shortened from the start, etc. "soilMoistureThreshold" -> "smt"
  String msg = "{\"soilMoistureThreshold\": "+String(soilMoistureThreshold)+", \"soilMoistureThreshold\": "+String(temperatureThreshold)+", \"activateFan\": "+String(activateFan)+"}";

  Serial.println("Sending Block 1 instructions");
  loraTxSerial.print("radio tx ");
  loraTxSerial.println(msg);
  checkTransmission();

  current_time = millis();
  if (current_time - start_time < TELEMETRY_INTERVAL * 2) {
   delay(TELEMETRY_INTERVAL-(current_time - start_time));
 }

  // --------------- receive data from block 3
  //RX switch to freq 3
  Serial.println("Set Rx to Block 3 FREQ");
  loraRxSerial.print("radio set freq ");
  loraTxSerial.println(FREQ_3);
  str = loraRxSerial.readStringUntil('\n');
  
  // --------------- receive data from block 3
  Serial.println("waiting for a message from block 3");
  data3=receiveData();

  //TX switch to freq 3
  Serial.println("Set Tx to Block 3 FREQ");
  loraTxSerial.print("radio set freq ");
  loraTxSerial.println(FREQ_3);
  str = loraTxSerial.readStringUntil('\n');  
  delay(3000);

  //send the pump instructions and check it is sent correctly
  msg = "{\"soilMoistureThreshold\": "+String(soilMoistureThreshold)+", \"soilMoistureThreshold\": "+String(temperatureThreshold)+", \"activatePump\": "+String(activatePump)+"}";

  Serial.println("Sending Block 3 instructions");
  loraTxSerial.print("radio tx ");
  loraTxSerial.println(msg);
  checkTransmission();
  
  //comunication with Cloud
  
  // Generate random values (for initial testing)
  //float humidity = random(2000, 2000) / 100.0;
  //float temperature = random(21, 21);
  //float SoilMoisture = random(9500, 9500) / 100.0;
  //int LightLevel = random(250, 250);
  
  if (!isConnected) {
    // Re-establish connection (UART Interrupt might be appropriate for further developments)
    client_reconnect();
  }

  if (isConnected) {
    unsigned long ms = millis();
    if (ms - lastTick > 10000) {  // send telemetry at most every 10 seconds - applicable for testing
      char msg[128] = {0};
      int pos = 0, errorCode = 0;

      lastTick = ms;

      // Send telemetry data in JSON format to cloud
      pos = snprintf(msg, sizeof(msg) - 1, "{\"temperature\": %f, \"humidity\": %f, \"SoilMoisture\": %f, \"LightLevel\": %d}", temperature, humidity, soilMoisture, lightLevel);
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
  
 // Sleep untill the cycle started exactly {CYCLE_TIME} ago
 current_time = millis();

Serial.print("Cycle took: ");
Serial.println(current_time - start_time);

Serial.println("Sleep for: ");
Serial.println(CYCLE_TIME-(current_time - start_time));

 if (current_time - start_time < CYCLE_TIME) { // only sleep if we have time to do so before next cycle is supposed to start
   delay(CYCLE_TIME-(current_time - start_time));
 }

};


// ----- Helper functions -----
// Setup WiFi
void wifi_setup () {
  Serial.println("setup wifi-connection");
  connect_wifi(WIFI_SSID, WIFI_PASSWORD);
  connect_client(SCOPE_ID, DEVICE_ID, DEVICE_KEY);

  if (context != NULL) {
    lastTick = 0;
  }
}

// Azure IoT Central will disconnect if too much time is spent without any data-transfer. This will be the case almost every time in our case.
void client_reconnect () { // This part is stil flawed... Cannot re-connect
  Serial.println("re-establishing cloud connection");
  // Reconnect:
  iotc_connect(context, SCOPE_ID, DEVICE_KEY, DEVICE_ID, 4);
  // ucertainty on implications of 'IOTConnectType'

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
      // Store value untill next itteration
      activateFan = 1;

      // Testing:
      //digitalWrite(FAN_LED_PIN, HIGH);
      //delay(FAN_ON_TIME);
      //digitalWrite(FAN_LED_PIN, LOW);

    } else if (strcmp(callbackInfo->tag, "ActivatePump") == 0) {

      // Activate Pump Downlink
      activatePump = 1;

      // Testing:
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
      Serial.println("transmission succesfull"); //Transmission failed
    }
  }
  else
  {
    Serial.println("transmission succesfull"); //Transmission failed
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

String receiveData(){
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
      data=str.substring(index+1); //keeps only the <data> part of the string
      //data= data_str.toInt(); //transforms the string into an int (i supposed it would be the data type we would need)
    }
    else
    {
      Serial.println("!no data recieved!");
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
  
  loraTxSerial.println("radio set sync 1"); //set the sync word used
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

