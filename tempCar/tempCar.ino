#include <RF24.h>
#include <RF24_config.h>
#include <nRF24L01.h>
#include <printf.h>

int in1 = 3;
int in2 = 4;

int in3 = 2;
int in4 = 5;

void moveForward(){
  digitalWrite(in1,HIGH);
  digitalWrite(in2,LOW);
  digitalWrite(in3,LOW);
  digitalWrite(in4,HIGH);
}
void stopMoving(){
  digitalWrite(in1,LOW);
  digitalWrite(in2,LOW);
  digitalWrite(in3,LOW);
  digitalWrite(in4,LOW);
}
void moveBack(){
  digitalWrite(in1,LOW);
  digitalWrite(in2,HIGH);
  digitalWrite(in3,HIGH);
  digitalWrite(in4,LOW);
}
void turnForwardRight(){
  digitalWrite(in1,HIGH);
  digitalWrite(in2,LOW);
  digitalWrite(in3,LOW);
  digitalWrite(in4,LOW);
}

void turnForwardLeft(){
  digitalWrite(in1,LOW);
  digitalWrite(in2,LOW);
  digitalWrite(in3,LOW);
  digitalWrite(in4,HIGH);
}
void turnBackRight(){
  digitalWrite(in1,LOW);
  digitalWrite(in2,HIGH);
  digitalWrite(in3,LOW);
  digitalWrite(in4,LOW);
}
void turnBackLeft(){
  digitalWrite(in1,LOW);
  digitalWrite(in2,LOW);
  digitalWrite(in3,HIGH);
  digitalWrite(in4,LOW);
}

// instantiate an object for the nRF24L01 transceiver
RF24 radio(7, 8);  // using pin 7 for the CE pin, and pin 8 for the CSN pin
 
// Let these addresses be used for the pair
const byte address[6] = "31516";
// It is very helpful to think of an address as a path instead of as
// an identifying device destination

long int time1;
   
typedef struct RemoteDataPackage{
  int dir = 6;
  int waterFlag = 0;
  char msg[10] = "New msg";
} RemoteDataPackage;

RemoteDataPackage globalData;

void setup() {
 
  Serial.begin(115200);
  while (!Serial) {
    // some boards need to wait to ensure access to serial over USB
  }
 
  // initialize the transceiver on the SPI bus
  if (!radio.begin()) {
    Serial.println(F("radio hardware is not responding!!"));
    while (1) {}  // hold in infinite loop
  }
 
  
  // Set the PA Level low to try preventing power supply related problems
  // because these examples are likely run with nodes in close proximity to
  // each other.
  radio.setPALevel(RF24_PA_MIN);  // RF24_PA_MAX is default.
  radio.setDataRate(RF24_250KBPS);
 
  // save on transmission time by setting the radio to only transmit the
  // number of bytes we need to transmit a float
  // radio.setPayloadSize(sizeof(payload));  // float datatype occupies 4 bytes
 
  // set the TX address of the RX node into the TX pipe
  radio.openWritingPipe(address);  // always uses pipe 0
 
  // set the RX address of the TX node into a RX pipe
  radio.openReadingPipe(1, address);  // using pipe 1
 
  radio.startListening();
 
  // For debugging info
  printf_begin();             // needed only once for printing details
  // radio.printDetails();       // (smaller) function that prints raw register values
  // radio.printPrettyDetails(); // (larger) function that prints human readable data

  time1 = millis();
 
}  // setup



void read(){
    // This device is a RX node
    RemoteDataPackage data;
    if (radio.available()) {              // is there a payload? get the pipe number that recieved it
      uint8_t bytes = radio.getPayloadSize();  // get the size of the payload     
      radio.read(&data, bytes);             // fetch payload from FIFO
      globalData.dir = data.dir;
      //Serial.print(F("Received "));
      //Serial.println(data.dir);
//      Serial.println(data.waterFlag);// print the payload's value
    }else{
      Serial.println(F("Read hoy na"));
      globalData.dir = 6;
    }
}

void move(int dir){
  if(dir == 6)
    stopMoving();
   else if(dir == 0)
    moveForward();
   else if(dir == 1)
    turnForwardLeft();
   else if(dir == 2)
    turnForwardRight();
   else if(dir == 4)
    turnBackLeft();
   else if(dir == 5)
    turnBackRight();
   else if(dir == 3)
    moveBack();
}
 
void loop() {
  
    
    if(radio.available()) {
      read();
      time1 = millis();
    }

    else{
      globalData.dir = 6;
    }
//    else if(millis() - time1 > 100){
//      globalData.dir = 6;
//      time1 = millis;
//    }
    Serial.println(globalData.dir);
    move(globalData.dir);
    delay(50);
 
}  // loop
