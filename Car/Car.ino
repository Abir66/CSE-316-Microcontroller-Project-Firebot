#include <RF24.h>
#include <RF24_config.h>
#include <nRF24L01.h>
#include <printf.h>

#define in1 3
#define in2 4
#define in3 2
#define in4 5

RF24 radio(7, 8);  // using pin 7 for the CE pin, and pin 8 for the CSN pin
uint8_t address[][6] = { "REMOT", "ROBOT" };

 
typedef struct {
	int dir = 6;
	bool water_mode = false;
} RemoteData;

typedef struct{
	float temp = 0.5;
	float gas = 0.3;
} CarData;

typedef struct{
	bool moving = false;
} CarState;

RemoteData remoteData;
CarData carData;
CarState carState;
volatile bool sensorReadTimeout = false;
volatile bool noInputTimeout = false;
volatile int noInputInturreptCounter = 0;
bool newData = false;


// -------------------- nrf functions --------------------
void sendData();
void receiveData();
void update();


// -------------------- car movement -----------------
void setToIdle();
void moveForward();
void stopMoving();
void moveBack();
void turnForwardRight();
void turnForwardLeft();
void turnBackRight();
void turnBackLeft();
void moveCar();
 
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

  	radio.enableAckPayload();
  
	// Set the PA Level low to try preventing power supply related problems
	// because these examples are likely run with nodes in close proximity to
	// each other.
	radio.setPALevel(RF24_PA_MIN);  // RF24_PA_MAX is default.
	radio.setDataRate(RF24_250KBPS);	
	radio.openWritingPipe(address[0]);  
	radio.openReadingPipe(1, address[1]);  
	radio.startListening();	
	cli();
	// --------------------------- Timer 1 for sensor data sending -------------------------------
	
	TCCR1A = 0;// set entire TCCR1A register to 0
	TCCR1B = 0;// same for TCCR1B
	TCNT1  = 0;//initialize counter value to 0
	// set compare match register for 1hz increments
	OCR1A = 31249;// = (16*10^6) / (1*1024) - 1 (must be <65536)  /// 16MHZ/(intended frequency * prescalar) - 1
	TCCR1B |= (1 << WGM12); // turn on CTC mode
	//TCCR1B |= (1 << CS12);  // Set CS12 bit for 256 prescaler
	TCCR1B |= (1 << CS12) | (1 << CS10);  // Set CS10 and CS12 bits for 1024 prescaler 
	TIMSK1 |= (1 << OCIE1A); // enable timer compare interrupt

	// --------------------------- Timer 2 for stoping car when no input -------------------------------
	TCCR2A = 0;// set entire TCCR2A register to 0
	TCCR2B = 0;// same for TCCR2B
	TCNT2  = 0;//initialize counter value to 0
	// set compare match register for 8khz increments
	OCR2A = 245;// = (16*10^6) / (freq*1024) - 1 (must be <256)
	// turn on CTC mode
	TCCR2A |= (1 << WGM21);
	// Set CS21 bit for 1024 prescaler
	TCCR2B |= (1 << CS12) | (1 << CS10);   
	// enable timer compare interrupt
	TIMSK2 |= (1 << OCIE2A);	


	sei();
 
}  // setup


ISR(TIMER1_COMPA_vect)      
{
	TCNT1 = 0;  // preload timer
	sensorReadTimeout = true;
}

ISR(TIMER2_COMPA_vect)      
{
	noInputInturreptCounter += 1;
	if(noInputInturreptCounter > 3){
		noInputTimeout = true;
		noInputInturreptCounter = 0;
	}

	TCNT2 = 0;
	
}


 
void loop() {
	if(sensorReadTimeout){
    	// read sensor and update carData
    	carData.gas += 1;
    
    	// write carData to remote
    	sendData();

    	// reset timer?
    	TCNT1 = 0;
    	sensorReadTimeout = false;
  	}

  	if(radio.available()) receiveData();
  	
	if(newData) update();
	else if(noInputTimeout) setToIdle();

}


void update(){
	Serial.print(F("Received :  "));
	Serial.print(remoteData.dir);
	Serial.print(" , ");
	Serial.println(remoteData.water_mode);
	newData = false;

	if(remoteData.water_mode){
		if(carState.moving) stopMoving();
	}

	else if(!remoteData.water_mode && remoteData.dir != 6){
		move();
	}

	noInputTimeout = false;
	noInputInturreptCounter = 0;
	TCNT2 = 0;
}

void setToIdle(){
	if(carState.moving) stopMoving();
	
}


void sendData(){
	radio.stopListening();
	unsigned long start_timer = micros();                
    bool report = radio.write(&carData, sizeof(carData));  
    radio.startListening();
    unsigned long end_timer = micros();                 
    if (report) {
    	Serial.print(F("Transmission successful! ")); 
    	Serial.print(F("Time to transmit = "));
    	Serial.print(end_timer - start_timer); 
    	Serial.print(F(" us. Sent: "));
    	Serial.println(carData.gas);
    	if ( radio.isAckPayloadAvailable() ) {
        	radio.read(&remoteData, sizeof(remoteData));
        	Serial.println("okay?");
        	newData = true;
    	}
    }
	else Serial.println(F("Transmission failed or timed out"));  // payload was not delivered
}


void receiveData(){
	if (radio.available()) {  
	radio.read(&remoteData, sizeof(remoteData));
	newData = true;
	}
}


// ------------------------------------------------- Car movement -------------------------------------------------
void move(){
	int dir = remoteData.dir;
	carState.moving = true;
	if(dir == 6) stopMoving();
	else if(dir == 0) moveForward();
	else if(dir == 1) turnForwardLeft();
	else if(dir == 2) turnForwardRight();
	else if(dir == 4) turnBackLeft();
	else if(dir == 5) turnBackRight();
	else if(dir == 3) moveBack();
}

void stopMoving(){
	digitalWrite(in1,LOW);
	digitalWrite(in2,LOW);
	digitalWrite(in3,LOW);
	digitalWrite(in4,LOW);
	carState.moving = false;
}

void moveForward(){
	digitalWrite(in1,HIGH);
	digitalWrite(in2,LOW);
	digitalWrite(in3,LOW);
	digitalWrite(in4,HIGH);
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