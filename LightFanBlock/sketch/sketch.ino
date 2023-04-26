// inspo: 
// Light Sensor setup https://projecthub.arduino.cc/DCamino/79c2ed93-e09e-47fd-93ff-fc9cf63d8a46
// Fan setup https://learn.adafruit.com/adafruit-arduino-lesson-13-dc-motors?view=all

#include <SoftwareSerial.h>

#define LORA_PINTX 7 // TX; connected to the lora RX (D6 in the ESP, change it if you use arduino) 
#define LORA_PINRX 8 // RX;

#define FREQ_2 864000000
#define FREQ_B 866000000

#define WATCHDOG 10000
#define SLEEP_TIME 10000

SoftwareSerial loraSerial(LORA_PINRX, LORA_PINTX);

String str, data_str;
int data, instr;

int light = 0; // store the current light value

int lightPin = 13;
int motorPin = 12;

void setup() {
    // put your setup code here, to run once:
    Serial.begin(57600); //configure  serial to talk to computer // 9600 // 57600
    pinMode(lightPin, OUTPUT); // configure digital pin  13 as an output
    //pinMode(12, OUTPUT); // configure digital pin 12 as an output

    pinMode(motorPin, OUTPUT);
    //while (! Serial);
    //Serial.println("Speed 0 to 255");

    setupLora();
    Serial.println("setup completed");

}

void  loop() {

    // put your main code here, to run repeatedly:
    light = analogRead(A0);  // read and save value from PR
    
    //Serial.println(light); // print current  light value
 
    if(light > 450) { // If it is bright...
        Serial.println("It  is quite light!");
        digitalWrite(lightPin,LOW); //turn left LED off
        //digitalWrite(12,LOW);  // turn right LED off
    }
    else if(light > 229 && light < 451) { // If  it is average light...
        Serial.println("It is average light!");
       digitalWrite(lightPin, HIGH); // turn left LED on
       //digitalWrite(12,LOW);  // turn right LED off
    }
    else { // If it's dark...
        Serial.println("It  is pretty dark!");
        digitalWrite(lightPin,HIGH); // Turn left LED on
        //digitalWrite(12,HIGH);  // Turn right LED on
    }

    // Replace serial monitor input w. Lora package instructions
    if (Serial.available())
    {
      Serial.println("Recieved Serial Input");
      int speed = Serial.parseInt();
      Serial.println(speed, DEC);
      if (speed >= 0 && speed <= 255)
      {
        analogWrite(motorPin, speed);
        delay(1000); //Run for a couple of seconds - the serial "end token" will return 0 and turn off the motor
      }
    }

    //delay(2000); // don't spam the computer!

  // Lora communication  
  //switch to freq 2
  loraSerial.println("radio set freq ");
  loraSerial.println(FREQ_B);
  str = loraSerial.readStringUntil('\n');
  // waiting for the sync message
  Serial.println("waiting for sync message");
  receiveSYNC();

  delay(4000);

  //switch to freq 2
  loraSerial.println("radio set freq ");
  loraSerial.println(FREQ_2);
  str = loraSerial.readStringUntil('\n');
  
  //send data to the master and check it is sent correctly
  Serial.println("sending data to the master");
  loraSerial.print("radio tx ");
  loraSerial.println("999"); //put here the data you want to send
  checkTransmission();

  //receive instructions from master block
  Serial.println("waiting for instructions");
  data=receiveData();
  Serial.println(data);
  
  //execute the instructions - manual Motor run, updated setting etc.

  delay(1000);


  //set the lora module in sleep mode untile the next iteration
  loraSerial.print("sys sleep "); //max sleep time is 4'294'967'296 ms
  loraSerial.println(SLEEP_TIME);
  str = loraSerial.readStringUntil('\n');
}

/*
After sending something using the command "radio tx <data>", the module prints two messages.
the first one is "ok" if the parameters are correct and the module is in tx mode, or something like "invalid param" if there is a problem. 
check the RN2483 user manual on dtu Learn, week 5 if you need the exact message
the second one is "radio_tx_ok" if the transmission was successfull
this function prints in the serial monitor "message sent correctly" if there are no errors, or "transmission failed"
*/
void checkTransmission() {
  str = loraSerial.readStringUntil('\n');
  delay(20);
  if ( str.indexOf("ok") == 0 ) //check if the parameters are correct, and we are in tx mode
  {
    str = String("");
    while(str=="")
    {
      str = loraSerial.readStringUntil('\n');
    }
    if ( str.indexOf("radio_tx_ok") == 0 )  //checking if data was sent correctly
    {
      Serial.println("message sent correctly"); //printing received data
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

//very similar to receiveData, but it does not save the data
//change the 1 after radio_rx if you change the sync message
void receiveSYNC(){
  
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
    if ( str.indexOf("radio_rx 321") == 0 )  //checking if the sync message is received 
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

void setupLora() {
  Serial.println("\nInitiating LoRa Setup");
  
  loraSerial.begin(57600);  // Serial communication to RN2483 // 9600
  loraSerial.setTimeout(5000);

  loraSerial.listen();
  
  loraSerial.println("sys get ver");
  str = loraSerial.readStringUntil('\n');
  Serial.println(str);
  
  loraSerial.println("mac pause"); //necessary before radio commands
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


