#include <RH_RF95.h>
#include <DHT.h>

#define DHTPIN 2     // DHT11 sensor data pin
#define DHTTYPE DHT11   // DHT11 sensor type
#define RFM95_CS 10
#define RFM95_RST 9
#define RFM95_INT 2
#define LED 13

DHT dht(DHTPIN, DHTTYPE);
RH_RF95 rf95(RFM95_CS, RFM95_INT);

// Define temperature and humidity thresholds
float temperatureThreshold = 25.0;
float humidityThreshold = 60.0;

void setup() {
  Serial.begin(9600);
  while (!Serial) delay(10);
  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);
  if (!rf95.init()) {
    Serial.println("LoRa radio init failed!");
    while (1);
  }
  if (!rf95.setFrequency(915.0)) {
    Serial.println("setFrequency failed");
    while (1);
  }
  rf95.setTxPower(23, false);
  dht.begin();
}

void loop() {
  delay(2000);
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.print("%  Temperature: ");
  Serial.print(temperature);
  Serial.println("°C");
  
  // Check if temperature or humidity is above threshold
  if (temperature >= temperatureThreshold || humidity >= humidityThreshold) {
    Serial.println("Threshold exceeded!");
    
    uint8_t data[4];
    data[0] = (uint8_t)humidity;
    data[1] = (uint8_t)(humidity >> 8);
    data[2] = (uint8_t)temperature;
    data[3] = (uint8_t)(temperature >> 8);
    
    digitalWrite(LED, HIGH);
    rf95.send(data, sizeof(data));
    rf95.waitPacketSent();
    digitalWrite(LED, LOW);
  }
}
