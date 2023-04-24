const int motorPin = 9;

// Moisture sensor
const int moisturePin = A0;
float moistureRead;
float moistureValue;
const int maximumMoistureValue = 1023;
const int moistureSetpoint = 40;

void setup()
  {
    pinMode(motorPin, OUTPUT);
    Serial.begin(9600);
    Serial.println("");
    Serial.println("Serial output ready =======================");
  }

void loop()
  {
    moistureRead = analogRead(moisturePin);
    moistureValue = moistureRead/maximumMoistureValue * 100;
    Serial.print("Moisture read: ");
    Serial.println(moistureRead);
    Serial.print("Moisture level: ");
    Serial.print(moistureValue);
    Serial.println("%");

    if (moistureValue < moistureSetpoint) 
      {
        digitalWrite(motorPin, HIGH);
      }
    else
      {
        digitalWrite(motorPin, LOW);
      }
    delay(1000);
  }