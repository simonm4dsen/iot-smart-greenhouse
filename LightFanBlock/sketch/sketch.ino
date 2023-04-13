// inspo: 
// Light Sensor setup https://projecthub.arduino.cc/DCamino/79c2ed93-e09e-47fd-93ff-fc9cf63d8a46
// Fan setup https://learn.adafruit.com/adafruit-arduino-lesson-13-dc-motors?view=all


int light = 0; // store the current light value

int lightPin = 13;
int motorPin = 12;

void setup() {
    // put your setup code here, to run once:
    Serial.begin(9600); //configure  serial to talk to computer
    pinMode(lightPin, OUTPUT); // configure digital pin  13 as an output
    //pinMode(12, OUTPUT); // configure digital pin 12 as an output

    pinMode(motorPin, OUTPUT);
    //while (! Serial);
    Serial.println("Speed 0 to 255");

}

void  loop() {

    // put your main code here, to run repeatedly:
    light = analogRead(A0);  // read and save value from PR
    
    //Serial.println(light); // print current  light value
 
    if(light > 450) { // If it is bright...
        //Serial.println("It  is quite light!");
        digitalWrite(lightPin,LOW); //turn left LED off
        //digitalWrite(12,LOW);  // turn right LED off
    }
    else if(light > 229 && light < 451) { // If  it is average light...
        //Serial.println("It is average light!");
       digitalWrite(lightPin, HIGH); // turn left LED on
       //digitalWrite(12,LOW);  // turn right LED off
    }
    else { // If it's dark...
        //Serial.println("It  is pretty dark!");
        digitalWrite(lightPin,HIGH); // Turn left LED on
        //digitalWrite(12,HIGH);  // Turn right LED on
    }

    if (Serial.available())
    {
      Serial.println("Recieved Serial Input");
      int speed = Serial.parseInt();
      Serial.println(speed, DEC);
      if (speed >= 0 && speed <= 255)
      {
        analogWrite(motorPin, speed);
        delay(2000); //Run for a couple of seconds - the serial "end token" will return 0 and turn off the motor
      }
    }

    delay(2000); // don't spam the computer!
}