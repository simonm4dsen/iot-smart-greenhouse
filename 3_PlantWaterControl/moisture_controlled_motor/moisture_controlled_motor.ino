
#define moisturePin A0

// Motor
const int motorPin = 2;
int motorState = 0;

// Soil moisture
float moistureValue = 0;
float moistureMaximum = 1023.00;
float moistureSetpoint = 99.00;

void setup() 
  {
  pinMode(motorPin, OUTPUT); 
  Serial.begin(9600); 
  }

void loop()
{
  moistureValue = analogRead(moisturePin)/moistureMaximum*100; 
  if (moistureValue < moistureSetpoint) {
    digitalWrite(motorPin, HIGH);
    motorState = 1;
  } else {
    digitalWrite(motorPin, LOW);
    motorState = 0;
  }
  Serial.print("Soil moisture: ");
  Serial.print(moistureValue);
  Serial.println("%");
  Serial.print("Motor state: ");
  Serial.println(motorState);
  delay(1000); 
}
