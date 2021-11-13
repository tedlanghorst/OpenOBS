/*
  Script 
*/

/*TODO

*/

//libraries
#include <Wire.h>               //standard library
#include <SPI.h>                //standard library
#include <LiquidCrystal_I2C.h>  //standard library
#include <Adafruit_ADS1X15.h>   //Version 2.2.0  https://github.com/adafruit/Adafruit_ADS1X15
#include <SdFat.h>              //Version 2.0.7 https://github.com/greiman/SdFat //uses 908 bytes of memory
#include <DS3231.h>             //Updated Jan 2, 2017 https://github.com/kinasmith/DS3231
 
//pins for LEDS
#define pLED_ADC 7
#define pLED_RTC 6
#define pLED_SD 5
#define pChipSelect 10

//RTC vars
DS3231 rtc; //create RTC object

//SD vars
#define SPI_SPEED SD_SCK_MHZ(50) 
SdFat sd;

//ADC vars
Adafruit_ADS1115 ads;

/* SETUP

 */

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(50);
  Wire.begin();

  pinMode(pLED_ADC, OUTPUT);
  pinMode(pLED_RTC, OUTPUT);
  pinMode(pLED_SD, OUTPUT); 
    
  
}

void loop() {

  //initialize the ADC
  ads.setGain(GAIN_ONE); //reset gain
  ads.begin(0x48);  // Initialize ads1115
  ads.setDataRate(RATE_ADS1115_860SPS); //set the sampling speed
  ads.readADC_SingleEnded(0); //throw one reading away. Seems to come up bad.
  bool adc_init = ads.readADC_SingleEnded(0) != -1;
  if(!adc_init) {
    Serial.println("ADC fail");
  } else {
    Serial.println("ADC pass");
    digitalWrite(pLED_ADC,HIGH);
  }
  
  //initialize the RTC
  bool clk_init = rtc.begin(); //reset the rtc
  uint32_t t_now = rtc.now().unixtime();
  clk_init = clk_init & t_now >= 946684800 & t_now < 947684800;
  if(!clk_init) {
    Serial.println("RTC failed");
  } else {
    Serial.println("RTC pass");
    digitalWrite(pLED_RTC,HIGH);
  }

  //intialize SD card
  bool sd_init = sd.begin(pChipSelect,SPI_SPEED);
  if(!sd_init) {
    Serial.println("SD failed");
  } else {
    Serial.println("SD pass");
    digitalWrite(pLED_SD,HIGH);
  }

  delay(900);
  digitalWrite(pLED_SD,LOW);
  digitalWrite(pLED_RTC,LOW);
  digitalWrite(pLED_ADC,LOW);
  delay(100);
}
