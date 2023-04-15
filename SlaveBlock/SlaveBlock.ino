#include <SoftwareSerial.h>

#define LORA_PINTX 12 //connected to the lora RX (D6)
#define LORA_PINRX 14 //connected to the lora TX (D5)
#define FREQ_2 864000000
#define FREQ_B 866000000
#define WATCHDOG 10000
#define SLEEP_TIME

SoftwareSerial loraSerial(LORA_PINRX, LORA_PINTX);

String str, data_str;
int data, instr;

void setup() {
  Serial.begin(57600);  // Serial communication to PC
  setupLora();
  Serial.println("setup completed");
}

void loop() {
  // waiting for the sync message
  Serial.println("waiting for sync message");
  receiveSYNC();

  delay(4000);

  //switch to freq 1
  loraSerial.println("radio set freq FREQ_2");
  str = loraSerial.readStringUntil('\n');
  
  //send data to the master and check it is sent correctly
  Serial.println("sending data to the master");
  loraSerial.print("radio tx ");
  loraSerial.println("999");
  checkTransmission();

  //receive instructions from master block
  Serial.println("waiting for instructions");
  data=receiveData();
  Serial.println(data);
  //execute the instructions

  delay(10000);


/*
  //set the lora module in sleep mode untile the next iteration
  loraSerial.println("sys sleep SLEEP_TIME"); //max sleep time is 4'294'967'296 ms
  str = loraSerial.readStringUntil('\n');
*/

}


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


void setupLora() {

  loraSerial.begin(9600);  // Serial communication to RN2483
  loraSerial.setTimeout(1000);

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

