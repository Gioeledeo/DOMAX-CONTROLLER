#include <Wire.h>
#include <SPI.h>
#include <RFID.h>
#include <LiquidCrystal_I2C.h>

String authCard[] = {}; //code of authorized card
int relaySignal[] = {23, 24, 25, 26, 27, 28}; //array output signal to relay
char* espOutput[] = {"A8","A9","A10","A11","A12","A13","A14","A15"}; //array input signal by esp8266 for relay control
char* w_switch[] = {"A0","A1", "A2", "A3", "A4", "A5", "A6", "A7"};
float distance[] = {0,0}; //initialize array for allarm proximity sensor
int relay_prevState[] = {0,0,0,0,0,0};

LiquidCrystal_I2C lcd(20, 4, 0x27);  // Set the LCD I2C address

//rfid
#define SDA_DIO 53
#define RESET_DIO 9
#define delayRead 500
RFID RC522(SDA_DIO, RESET_DIO); //set RC522 ICSP adress

//allarm controller
#define alm_TRIG 10 //HC-SR04 Trig pin
#define alm_ECHO 9 //HC-SR04 echo pin
#define alm_SIREN 22 //relay of siren
#define alm_STOP espOutput[6] //pin connected to esp8266 to disable allarm by remote device

int alm_OFF,cardSet_Done;
String admin_CARD;

void setup(){
  lcd.init();
  intialDisplay();
  systemSetup();

  Serial.begin(9600);
  SPI.begin();
  RC522.init(); //rfid controller library

  pinMode(alm_TRIG, OUTPUT);
  pinMode(alm_ECHO, INPUT);
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
  if(analogRead(w_switch[0])>=500){
    allarm();
  }
  rfid_control();
  relay_control();
}

void allarm(){
  for(int i = 0; i < 2; i++){
    digitalWrite(alm_TRIG, HIGH);
    delay(10);
    digitalWrite(alm_TRIG,LOW);
    unsigned long time = pulseIn(alm_ECHO, HIGH);
    distance[i] = .03438 * time / 2;
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

    if(analogRead(espOutput[i]) >= 500 && analogRead(w_switch[i] >= 500 && relay_prevState[i] == 0)){
      digitalWrite(relaySignal[i], LOW); //active low !!
      relay_prevState[i] = relay_prevState +1;
      
      intialDisplay();
      lcd.setCursor(0,1);
      lcd.print("Signal received for:");
      lcd.setCursor(0,2);
      lcd.print("RELAY ");
      lcd.print(i);
      lcd.print(" - state: ON");
    }

    else if(analogRead(espOutput[i]) < 400 || analogRead(w_switch[i]) < 400){
      digitalWrite(relaySignal[i], HIGH);
      relay_prevState[i] = 0;
      
      intialDisplay();
      lcd.setCursor(0,1);
      lcd.print("Signal received for:");
      lcd.setCursor(0,2);
      lcd.print("RELAY ");
      lcd.print(i);
      lcd.print(" - state:OFF");
    }
  }
}

void cardManager(){
  intialDisplay();
  cardSet_Done = 0;
  byte i;

  do{
    int memCard = 3 - cardSet_Done;
    lcd.setCursor(3,1);
    lcd.print("present card");
    lcd.setCursor(1,2);
    lcd.print("remain: ");
    lcd.print(memCard);
    lcd.print(" card/s");

    if(RC522.isCard()){
      RC522.readCardSerial();
      String readCode = "";

      for(int i = 0; i < 4; i++){
        readCode+= String (RC522.serNum[i], HEX);
        readCode.toUpperCase();
        authCard[i] = readCode;
      }
      cardSet_Done = cardSet_Done+1;
    }
  }
  while(cardSet_Done != 3);

  admin_CARD = authCard[0];

  intialDisplay();
  lcd.setCursor(1,1);
  lcd.print("auth-cards stored.");
  lcd.setCursor(0,2);
  lcd.print("admin card: ");
  lcd.print(admin_CARD);
}

void intialDisplay(){
  lcd.clear();
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("- DOMAX CONTROLLER -");
  lcd.setCursor(1,3);
  lcd.print("dev. by gioele.deo");
}

void systemSetup(){}