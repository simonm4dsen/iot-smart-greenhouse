#include <SoftwareSerial.h>

#define LORAT_TX 1
#define LORAT_RX 1
#define LORAR_TX 1
#define LORAR_RX 1
#define FREQ_1
#define FREQ_2
#define FREQ_3
#define FREQ_B
#define WHATCHDOG

SoftwareSerial loraTxSerial(LORAT_RX, LORAT_TX);
SoftwareSerial loraRxSerial(LORAR_RX, LORAR_TX);

String str, data_str;
int data1, data2, data3;

void setup() {
  Serial.begin(57600);  // Serial communication to PC
  setupLora(loraTxSerial);
  setupLora(loraRxSerial);

  Serial.println("setup completed");
}

void loop() {
  // communication with the modules
  setupLora(loraTxSerial) //default frequency is broadcast
  setupLora(loraRxSerial)

  //send the syncronization message and check it is sent correctly
  Serial.println("sending sync message");
  loraTxSerial.print("radio tx ");
  loraTxSerial.println("SYNC");
  checkTransmission();

  //RX switch to freq 1
  loraRxSerial.println("radio set freq FREQ_1");
  str = loraRxSerial.readStringUntil('\n');
  
  //receive data from block 1
  Serial.println("waiting for a message from block 1");
  data1=receiveData();

  delay(-----)

  //RX switch to freq 2
  loraRxSerial.println("radio set freq FREQ_2");
  str = loraRxSerial.readStringUntil('\n');
  
  //receive data from block 2
  Serial.println("waiting for a message from block 2");
  data2=receiveData();
  
  //compute instructions for the fan using data1 and data2



  //TX switch to freq 2
  loraTxSerial.println("radio set freq FREQ_2");
  str = loraTxSerial.readStringUntil('\n');  

  //send the fan instructions and check it is sent correctly
  Serial.println("sending fan instructions");
  loraTxSerial.print("radio tx ");
  loraTxSerial.println("------");
  checkTransmission();

  delay(-----)

  //RX switch to freq 3
  loraRxSerial.println("radio set freq FREQ_3");
  str = loraRxSerial.readStringUntil('\n');
  
  //receive data from block 3
  Serial.println("waiting for a message from block 3");
  data3=receiveData();

  //compute instructions for the pump using data1, data2 and data3


  //TX switch to freq 3
  loraTxSerial.println("radio set freq FREQ_3");
  str = loraTxSerial.readStringUntil('\n');  

  //send the pump instructions and check it is sent correctly
  Serial.println("sending pump instructions");
  loraTxSerial.print("radio tx ");
  loraTxSerial.println("------");
  checkTransmission();
  
  delay(-------)
  
  setupLoraWAN(loraTxSerial)
  setupLoraWAN(loraRxSerial)
  
  //comunication with the server

}


void checkTransmission() {
  str = loraTxSerial.readStringUntil('\n');
  delay(20);
  if ( str.indexOf("ok") == 0 ) //check if the parameters are correct, and we are in tx mode
  {
    str = String("");
    while(str=="")
    {
      str = loraTxSerial.readStringUntil('\n');
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

int receiveData(){
  int data=0;
  loraRxSerial.println("radio rx 0"); //wait for to receive until the watchdogtime
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


void setupLora(SoftwareSerial &loraSerial) {

  loraSerial.begin(9600);  // Serial communication to RN2483
  loraSerial.setTimeout(1000);

  loraSerial.listen();
  str = loraSerial.readStringUntil('\n');
  Serial.println(str);
  loraSerial.println("sys get ver");
  str = loraSerial.readStringUntil('\n');
  Serial.println(str);
  
  loraSerial.println("mac pause"); //necessary before radio commands
  str = loraSerial.readStringUntil('\n');
  Serial.println(str);  

  loraSerial.println("radio set mod lora");
  str = loraSerial.readStringUntil('\n');
  Serial.println(str);

  loraSerial.println("radio set freq FREQ_B");
  str = loraSerial.readStringUntil('\n');
  Serial.println(str);

  loraSerial.println("radio set pwr 3");  //max power 14 dBm, -3 is the min, 3 good for power saving
  str = loraSerial.readStringUntil('\n');
  Serial.println(str);

  loraSerial.println("radio set sf sf7"); //min range, fast data rate, minimum battery impact
  str = loraSerial.readStringUntil('\n');
  Serial.println(str);
  
  loraSerial.println("radio set afcbw 41.7"); //sets the value used by the automatic frequency correction bandwidth
  str = loraSerial.readStringUntil('\n');
  Serial.println(str);
  
  loraSerial.println("radio set rxbw 125"); //Lower receiver BW equals better link budget / SNR (less noise), but htere can be problems of freq drifting
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
  
  loraSerial.println("radio set wdt WATCHDOG"); //set the whatch dog timer,  disable for continuous reception
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

void setupLoraWAN() {
  
}
