#include <Wire.h>
#include <SPI.h>
#include <RFID.h>

char authCard = {}; //code of authorized card
const int relaySignal = {23, 24, 25, 26, 27, 28}; //array output signal to relay
char espOutput = {"A8","A9","A10","A11","A12","A13","A14","A15"}; //array input signal by esp8266 for relay control


//rfid
#define SDA_DIO 
#define RESET_DIO
#define delayRead 500
RFID RC522(SDA_DIO, RESET_DIO);

//allarm controller
#define alm_EN A0
#define alm_TRIG 10
#define alm_ECHO 9
#define alm_SIREN 
#define alm_STOP A //pin connected to esp8266 to disable allarm by remote device

int distance[] = {0,0}; //initialize array for allarm proximity sensor

int alm_OFF,cardSet_Done;
char admin_CARD;

void setup(){
	Serial.begin(9600);
	SPI.begin();
	RC522.init(); //rfid controller library
	pinMode(alm_TRIG, OUTPUT);
	pinMode(alm_ECHO, INPUT);
	pinMode(alm_EN, INPUT);
	pinMode(alm_SIREN, OUTPUT);
	
	for(int i = 0; i < 9; i++){
		pinMode(relaySignal[i], OUTPUT); 
		digitalWrite(relaySignal[i], HIGH); //active low
	}

	digitalWrite(alm_TRIG, LOW);
	digitalWrite(alm_SIREN, HIGH); //siren relay is active low
	cardManager();
}

void loop(){
	if(analogRead(alm_EN)>=500){
		allarm();
	}
	rfid_control();
	relay_control();
}

void allarm(){
	for(int i = 0; i < 2; i++){
		digitalWrite(alm_TRIG, HIGH);
		delayMicrosecond(10);
		digitalWrite(alm_TRIG,LOW);
		unsigned long time = pulseIn(alm_ECHO, HIGH);
		float distance[i] = .03438 * time / 2;
		delay(500);
	}
	
	if(distance[0] != distance[1]){
		do{
			digitalWrite(alm_SIREN, LOW); //active low !!
			rfid_control();
		}
		while(alm_OFF != 1 || analogRead(alm_STOP)<500);
	}
}

void rfid_control(){
	byte i;
	if(RC522.isCard()){
		RC522.readCardSerial();
		String readCode = "";
		Serial.print("Card code: ");
		
		for(int i = 0; i < 4; i++){
			readCode+= String (RC522.serNum[i], HEX);
			readCode.toUpperCase();
		}
		Serial.println(readCode);

		for(int i = 0; i < 3; i++){
			if(readCode == authCard[i]){
				alm_OFF = 1;
			}
		}
	}
}

void relay_control(){
	for(int i = 0; i < 9; i++){
		if(analogRead(espOutput[i])>=500){
			digitalWrite(relaySignal[i], LOW); //active low !!
		}
	}
}

void cardManager(){
	cardSet_Done = 0;
	byte i;

	do{
		if(RC522.isCard()){
			RC522.readCardSerial();
			String readCode = "";

			for(int i = 0; i < 4; i++){
				readCode+= String (RC522.serNum[i], HEX);
				readCode.toUpperCase();
				authCard[i] = readCode;
			}
		}
		cardSet_Done = cardSet_Done+1;
	}
	while(cardSet_Done == 0);

	admin_CARD = authCard[0];
}