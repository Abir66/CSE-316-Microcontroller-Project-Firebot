#include <RF24.h>
#include <RF24_config.h>
#include <nRF24L01.h>
#include <printf.h>

#define joyX A0
#define joyY A1
#define joyButton 9
int car_code, button;

// instantiate an object for the nRF24L01 transceiver
RF24 radio(7, 8);  // using pin 7 for the CE pin, and pin 8 for the CSN pin
 
// Let these addresses be used for the pair
const byte address[6] = "31516";
// It is very helpful to think of an address as a path instead of as
// an identifying device destination
   
typedef struct{
  int dir = 6;
  int waterFlag = 0;
  char msg[10] = "New msg";
} RemoteDataPackage;

void joystick_setup() {
  pinMode(joyButton, INPUT);
  digitalWrite(joyButton, HIGH);
}

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
  radio.setPALevel(RF24_PA_MAX);  // RF24_PA_MAX is default.
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
 joystick_setup();
}  // setup


bool write(RemoteDataPackage data){

  radio.stopListening();
  unsigned long start_timer = micros();                // start the timer
  bool report = radio.write(&data, sizeof(data));  // transmit & save the report
  unsigned long end_timer = micros();                  // end the timer
  radio.startListening();
 
  if (report) {
    Serial.print(F("Transmission successful! "));  // payload was delivered
    Serial.print(F("Time to transmit = "));
    Serial.println(end_timer - start_timer);  // print the timer result
    Serial.println(data.dir);
    
  } else {
    Serial.println(F("Transmission failed or timed out"));  // payload was not delivered
  }
 
  return report;
}

int get_joystick_reading() {
  int code = 6;

  int xCoordinate = analogRead(joyX)/50;
  int yCoordinate = analogRead(joyY)/50;  

  code = 0;

  if (yCoordinate > 17) 
    code += 1;
  else if (yCoordinate <4)
    code +=2;
  
  if (xCoordinate > 13)
    code += 3;
  else if (xCoordinate > 7)
    code = 6;

  return code;
}
int is_joystick_button_pressed() {
  return !digitalRead(joyButton);
}


void loop() {
    car_code = get_joystick_reading();
    button = is_joystick_button_pressed();
    RemoteDataPackage data;
    data.dir = car_code;
    data.waterFlag = button;
    
    write(data);
    delay(20);   
 
}  // loop
