#include <SoftwareSerial.h>

#define LORAT_PINTX 12 //connected to the lora RX (D6)
#define LORAT_PINRX 14 //connected to the lora TX (D5)
#define LORAR_PINTX 4 //connected to the lora RX (D2)
#define LORAR_PINRX 5 //connected to the lora TX (D1)
#define FREQ_1 863000000
#define FREQ_2 864000000
#define FREQ_3 865000000
#define FREQ_B 866000000
#define WATCHDOG 10000

SoftwareSerial loraTxSerial(LORAT_PINRX, LORAT_PINTX);
SoftwareSerial loraRxSerial(LORAR_PINRX, LORAR_PINTX);

String str, data_str;
int data1, data2, data3;

void setup() {
  Serial.begin(57600);  // Serial communication to PC
  setupLoraRx();
  setupLoraTx();

  Serial.println("setup completed");
}

void loop() {
  // communication with the modules
/*  
  setupLora(loraTxSerial); //default frequency is broadcast
  setupLora(loraRxSerial);
*/  

  //send the syncronization message and check it is sent correctly
  Serial.println("sending sync message");
  loraTxSerial.print("radio tx ");
  loraTxSerial.println(1);
  checkTransmission();

/*  
  //RX switch to freq 1
  loraRxSerial.println("radio set freq FREQ_1");
  str = loraRxSerial.readStringUntil('\n');
  
  //receive data from block 1
  Serial.println("waiting for a message from block 1");
  data1=receiveData();
*/  
  delay(4000);


  //RX switch to freq 2
  //loraRxSerial.listen();  
  loraRxSerial.print("radio set freq ");
  loraRxSerial.println(FREQ_2);
  str = loraRxSerial.readStringUntil('\n');
  
  //receive data from block 2
  Serial.println("waiting for a message from block 2");
  data2=receiveData();
  Serial.println(data2);
  
  //compute instructions for the fan using data1 and data2



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
/*
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
*/  

}


void checkTransmission() {
  str = loraTxSerial.readStringUntil('\n');
  Serial.println(str);
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
      data= data_str.toInt(); //transforms the string into an int
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
  loraTxSerial.begin(9600);  // Serial communication to RN2483
  loraTxSerial.setTimeout(1000);

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
  
  loraTxSerial.println("radio set sync 12"); //set the sync word used
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
  loraRxSerial.begin(9600);  // Serial communication to RN2483
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
  loraRxSerial.println(FREQ_1);
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
  
  loraRxSerial.println("radio set sync 12"); //set the sync word used
  str = loraRxSerial.readStringUntil('\n');
  Serial.println(str);
  
  //For a fixed SF, a narrower bandwidth will increase sensitivity as the bit rate is reduced
  //this means more time-on-air, so more battery consumption, but it's easier to receive
  loraRxSerial.println("radio set bw 125");
  str = loraRxSerial.readStringUntil('\n');
  Serial.println(str);
}

void setupLoraWAN() {
  
}
