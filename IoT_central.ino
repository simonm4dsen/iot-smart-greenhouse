#include <ESP8266WiFi.h>
#include "src/iotc/common/string_buffer.h"
#include "src/iotc/iotc.h"
#include <ArduinoJson.h>

#define WIFI_SSID "Bethlenet_2000"
#define WIFI_PASSWORD "Stellanet"

#define FAN_LED_PIN D1
#define PUMP_LED_PIN D2
#define FAN_ON_TIME 10000 // 10 seconds
#define PUMP_ON_TIME 15000 // 15 seconds

const char* SCOPE_ID = "0ne009E225E";
const char* DEVICE_ID = "1hihltp023e";
const char* DEVICE_KEY = "+9VWNWUEuL6C6XeNDI9jkw4+b8S8Fa/vpuSr54qvOp8=";

const unsigned long TELEMETRY_INTERVAL = 5000;

float temperatureThreshold = 30.0;
float soilMoistureThreshold = 50.0;


unsigned long fanOffTime = 0;
unsigned long pumpOffTime = 0;

unsigned long lastPropertyReportTime = 0;
const unsigned long PROPERTY_REPORT_INTERVAL = 60000; // 1 minute


void on_event(IOTContext ctx, IOTCallbackInfo* callbackInfo);
#include "src/connection.h"


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
      digitalWrite(FAN_LED_PIN, HIGH);
      delay(FAN_ON_TIME);
      digitalWrite(FAN_LED_PIN, LOW);
    } else if (strcmp(callbackInfo->tag, "ActivatePump") == 0) {
      digitalWrite(PUMP_LED_PIN, HIGH);
      delay(PUMP_ON_TIME);
      digitalWrite(PUMP_LED_PIN, LOW);
    } else if (strcmp(callbackInfo->tag, "TemperatureThreshold") == 0) {
      float newThreshold = atof(*buffer);
      if (newThreshold >= 0) {
        temperatureThreshold = newThreshold;
        LOG_VERBOSE("Temperature threshold updated to %f", temperatureThreshold);
      }
    } else if (strcmp(callbackInfo->tag, "SoilMoistureThreshold") == 0) {
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





void setup() {
  Serial.begin(9600);

  pinMode(FAN_LED_PIN, OUTPUT);
  pinMode(PUMP_LED_PIN, OUTPUT);
  digitalWrite(FAN_LED_PIN, LOW);
  digitalWrite(PUMP_LED_PIN, LOW);

  connect_wifi(WIFI_SSID, WIFI_PASSWORD);
  connect_client(SCOPE_ID, DEVICE_ID, DEVICE_KEY);

  if (context != NULL) {
    lastTick = 0;
  }
}



void loop() {
  // Generate random values
  float humidity = random(2000, 8000) / 100.0;
  float temperature = random(0, 50);
  float SoilMoisture = random(0, 10000) / 100.0;
  int LightLevel = random(0, 1000);

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
        Serial.print("Temperature threshold: ");
        Serial.println(temperatureThreshold);
        Serial.print("Soil moisture threshold: ");
        Serial.println(soilMoistureThreshold);
    }
  }

  iotc_do_work(context);
}
