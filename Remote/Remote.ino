#include <RF24.h>
#include <RF24_config.h>
#include <nRF24L01.h>
#include <printf.h>

#define joyX A0
#define joyY A1
#define joyButton 9

int car_code, button;

// This will be the remote

// instantiate an object for the nRF24L01 transceiver
RF24 radio(7, 8);  // using pin 7 for the CE pin, and pin 8 for the CSN pin
 
// Let these addresses be used for the pair
uint8_t address[][6] = { "REMOT", "ROBOT" };
// It is very helpful to think of an address as a path instead of as
// an identifying device destination 
 
typedef struct {
  int dir = 6;
  bool water_mode = false;
} RemoteData;

typedef struct{
  float temp = 0.5;
  float gas = 0.3;
} CarData;


RemoteData remoteData;
CarData carData;

bool prev_joy_button_state = true;
bool is_joy_button_pressed = false;
long joystick_reading_time = millis();
long joy_button_press_time = millis();


void sendData();
void receiveData();
void showData();
void joystick_setup();
void get_joystick_reading();
void get_joystick_button_pressed();

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

  // AckPayload
  radio.enableAckPayload();
  radio.writeAckPayload(0, &remoteData, sizeof(remoteData));

  radio.openWritingPipe(address[1]); 
  radio.openReadingPipe(1, address[0]); 
  radio.startListening();


  joystick_setup();

  
}  // setup

bool check_change(){
  
  get_joystick_button_pressed();
  get_joystick_reading();

  if(is_joy_button_pressed || (millis() - joystick_reading_time > 50 && remoteData.dir != 6))  
    return true;
    
  return false;
}


void loop() {
  if(check_change()) sendData();
  if (radio.available()) receiveData();
}  // loop


void joystick_setup() {
  pinMode(joyButton, INPUT);
  digitalWrite(joyButton, HIGH);
}

void get_joystick_reading() {
  
  int xCoordinate = analogRead(joyX)/50;
  int yCoordinate = analogRead(joyY)/50;  

  int code = 0;

  if (yCoordinate > 17) code += 1;
  else if (yCoordinate <4) code +=2;
  
  if (xCoordinate > 13) code += 3;
  else if (xCoordinate > 7) code = 6;
  
  remoteData.dir = code;
}


void get_joystick_button_pressed() {

  if(millis() - joy_button_press_time < 20){
    is_joy_button_pressed = false;
    return;
  }
  
  bool current_joy_button_state = digitalRead(joyButton);
  if(!current_joy_button_state && prev_joy_button_state) {
    is_joy_button_pressed = true;
    remoteData.water_mode = !remoteData.water_mode;
  }
  else is_joy_button_pressed = false;
  prev_joy_button_state = current_joy_button_state;
  joy_button_press_time = millis();
}


void sendData(){
  unsigned long start_timer = micros();                // start the timer
  radio.stopListening();
  bool report = radio.write(&remoteData, sizeof(remoteData));  // transmit & save the report
  radio.startListening();
  unsigned long end_timer = micros();                  // end the timer
 
  if (report) {
    // Serial.print(F("Transmission successful! "));  // payload was delivered
    // Serial.print(F("Time to transmit = "));
    // Serial.print(end_timer - start_timer);  // print the timer result
    // Serial.print(F(" us. Sent: "));
    // Serial.print(remoteData.dir);  // print payload sent
    // Serial.print(" : ");
    // Serial.println(remoteData.water_mode);
    
  } 
    else {
      Serial.println(F("Transmission failed or timed out"));
    }
}

void receiveData(){
    if (radio.available()) {             
      radio.read(&carData, sizeof(carData));
      showData();
    }
}

void showData(){
      Serial.print(F("Received : "));
      Serial.print(carData.gas);  // print the payload's value
      Serial.print(F(" , "));
      Serial.println(carData.temp);  // print the payload's value
}


