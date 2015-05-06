#include <SPI.h>
#include <BLEPeripheral.h>
#include <Adafruit_NeoPixel.h>
#include <avr/power.h>

#define BLE_REQ 6
#define BLE_RDY 7
#define BLE_RST 4

#define VIBEONE 9
#define VIBETWO 10
#define VIBETHREE 11
#define FSR_PIN A0

#define NEO_PIN 8
#define NUM_NEOS 33

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_NEOS, NEO_PIN, NEO_GRB + NEO_KHZ800);

// timing variables to send fsr values
long previousMillis = 0;
long interval = 1000;

boolean bieber=false;
int bieber_counter=0;

boolean email=false;
int email_counter=0;

boolean preset1 = false;
boolean preset2 = false;
boolean preset3 = false;
boolean preset4 = false;
boolean neos_off = false;

int currentPreset = 0;
int brightnessFactor = 2;

uint32_t preset1Color = strip.Color(255/brightnessFactor,0,0);
uint32_t preset2Color = strip.Color(0,255/brightnessFactor,0);
uint32_t preset3Color = strip.Color(148/brightnessFactor,0,211/brightnessFactor);
uint32_t preset4Color = strip.Color(255/brightnessFactor,0,255/brightnessFactor);
uint32_t twitterColor = strip.Color(64/brightnessFactor,153/brightnessFactor,255/brightnessFactor);
uint32_t emailColor = strip.Color(255/brightnessFactor,215/brightnessFactor,0);
uint32_t offColor = strip.Color(0,0,0);

int lowActivity = 0; // new code
int highActivity = 1; //new code



BLEPeripheral shoe = BLEPeripheral(BLE_REQ, BLE_RDY, BLE_RST);


// Light attributes
BLEService ledService = BLEService("fff0");
BLECharCharacteristic ledCharacteristic = BLECharCharacteristic("fff1", BLERead | BLEWrite);

// Motor attributes
BLEService vibeService = BLEService("fff2");
BLECharCharacteristic vibeCharacteristic = BLECharCharacteristic("fff3", BLERead | BLEWrite);

// FSR attribute
BLEService activityService = BLEService("fff4");
//BLECharCharacteristic activityCharacteristic = BLECharCharacteristic("fff5", BLERead | BLENotify);
BLECharCharacteristic activityLevelCharacteristic = BLECharCharacteristic("fff5", BLERead | BLENotify); // new code

void setup() {

  Serial.begin(115200);
  
#if defined (__AVR_ATmega32U4__)
  delay(5000);  //5 seconds delay for enabling to see the start up comments on the serial board
#endif
  
  // Neopixel initiation
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  
  // set motor pins
  pinMode(VIBEONE, OUTPUT);
  pinMode(VIBETWO, OUTPUT);
  pinMode(VIBETHREE, OUTPUT);

  ////////// BLE PROPERTIES //////////
  shoe.setDeviceName("Lumen");
  shoe.setLocalName("lumen-shoe");
  shoe.setAdvertisedServiceUuid(ledService.uuid());
  
  // add light attributes
  shoe.addAttribute(ledService);
  shoe.addAttribute(ledCharacteristic);
  
  // add vibe attributes
  shoe.addAttribute(vibeService);
  shoe.addAttribute(vibeCharacteristic);
  
  // add activity attributes
  shoe.addAttribute(activityService);
  //shoe.addAttribute(activityCharacteristic);
  shoe.addAttribute(activityLevelCharacteristic);
  
  // ble event handlers
  shoe.setEventHandler(BLEConnected, shoeConnectHandler);
  shoe.setEventHandler(BLEDisconnected, shoeDisconnectHandler);

  // characterisitic event handlers
  ledCharacteristic.setEventHandler(BLEWritten, ledCharacteristicWritten);
  vibeCharacteristic.setEventHandler(BLEWritten, vibeCharacteristicWritten);
  
  shoe.begin(); 
}


void loop() {
  shoe.poll();
 
  // sending fsr reading every one second
  unsigned long currentMillis = millis();
  
  
  if(shoe.connected()){
    
    if(currentMillis - previousMillis > interval){
       previousMillis = currentMillis;
      
       int analogValue = analogRead(FSR_PIN);
       
       //activityCharacteristic.setValue(analogValue); //new code
       
       // send to phone to know if shoe is on or off
       if(analogValue <= 100){ //new code
         activityLevelCharacteristic.setValue(lowActivity);
       } else {
         activityLevelCharacteristic.setValue(highActivity);
       }

       Serial.println(analogValue);
    }
    
    
  }
}


////// EVENT HANDLERS //////

//connected to central
void shoeConnectHandler(BLECentral& central){
  Serial.println("connected to central");
}

//disconnected from central
void shoeDisconnectHandler(BLECentral& central){
   Serial.println("disconnected from central"); 
}

//led event handler
void ledCharacteristicWritten(BLECentral& central, BLECharacteristic& characteristic){
  
  switch(ledCharacteristic.value()){
     case 'q':
       Serial.println("Set preset 1");
      colorWipe(preset1Color,50);
      currentPreset = 1;
      break;
      
     case 'w':
       Serial.println("set Preset 2");
       colorWipe(preset2Color,50);
       currentPreset = 2;
       break;
      
     case 'e':
       Serial.println("set preset 3");
      colorWipe(preset3Color,50);
      currentPreset = 3;
       break;
     
     case 'r':
       Serial.println("set preset 4");
      colorWipe(preset4Color,50); 
      currentPreset = 4;
       break; 
       
     case 't':
       Serial.println("10 bieber tweets");
       theaterChase(twitterColor,50);
       break; 
       
     case 'y':
       Serial.println("got an email");
       theaterChase(emailColor,50);
       break; 
       
     case 'o':
       Serial.println("turn leds off");
       colorWipe(offColor,20);
       break;
  }
}

//vibe event handler
void vibeCharacteristicWritten(BLECentral& central, BLECharacteristic& characteristic){
   Serial.print("vibe char received: ");
   Serial.println(vibeCharacteristic.value()); 
   
   //6D in hex
   if(vibeCharacteristic.value() == 'm'){
      Serial.println("Move motor move"); 
   }
}

void rainbow(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256; j++) {
    for(i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel((i+j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

//Theatre-style crawling lights.
void theaterChase(uint32_t c, uint8_t wait) {
  for (int j=0; j<8; j++) {  //do 8 cycles of chasing
    for (int q=0; q < 3; q++) {
      for (int i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, c);    //turn every third pixel on
      }
      strip.show();
     
      delay(wait);
     
      for (int i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
  
  return_to_prev_preset(currentPreset);
}

void return_to_prev_preset(int preset_num){
    switch(preset_num){
       case 1:
         colorWipe(preset1Color,50);
         break;

       case 2:
         colorWipe(preset2Color,50);
         break;
       
       case 3:
         colorWipe(preset3Color,50);
         break;
         
       case 4:
         colorWipe(preset4Color,50);
         break;
    }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
   return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else if(WheelPos < 170) {
    WheelPos -= 85;
   return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  } else {
   WheelPos -= 170;
   return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  }
}

void colorWipe(uint32_t c,uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, c);
      strip.show();
      delay(wait);
  }
}

// turn all neo pixels off
void turn_neos_off(){
  for(uint16_t i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, strip.Color(0,0,0));
      strip.show();
  } 
}


