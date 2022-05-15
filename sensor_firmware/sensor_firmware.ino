/*
  Test strings for serial monitor
  $OPENOBS*4A
  $SET,1616605996,10*7E
*/

/*TODO

*/

/*LIBRARIES
 *  The first three libraries are included in standard Arduino IDE installations
 *  The last three libraries should be be downloaded from github and installed manually.
 *  Additionally, the Adafruit_ADS1015 library requires the Adafruit_I2CDevice library, if you do not already have it.
 */
#include <Wire.h>               //standard library
#include <SPI.h>                //standard library
#include <EEPROM.h>             //standard library
#include <Adafruit_ADS1X15.h>   //Version 2.2.0  https://github.com/adafruit/Adafruit_ADS1X15
#include <SdFat.h>              //Version 2.0.7 https://github.com/greiman/SdFat //uses 908 bytes of memory
#include <DS3231.h>             //Updated Jan 2, 2017 https://github.com/kinasmith/DS3231

/*
 *  CONFIGURATION SETTINGS
 */
 
//firmware data
const DateTime uploadDT = DateTime((__DATE__),(__TIME__)); //saves compile time into progmem
const char contactInfo[] PROGMEM = "if found, contact efe@unc.edu"; 
const char dataColumnLabels[] PROGMEM = "time,millis,R0,gain,temp,batt";
uint16_t serialNumber;

//sampling constants
const uint16_t NUM_BACKGROUND = 100;
const uint16_t NUM_SAMPLES = 1000;

//connected pins
#define pVoltageDivider 4    //voltage divider
#define pIRED A3             //IR emitter
#define pChipSelect 10       //chip select pin for SD card

//EEPROM addresses
#define SLEEP_ADDRESS 0
#define SN_ADDRESS 500
#define UPLOAD_TIME_ADDRESS 502

//communications vars
bool guiConnected = false;
const uint16_t COMMS_WAIT = 500;   //ms delay to try gui connection
const int MAX_CHAR = 60;            //max num character in messages
char messageBuffer[MAX_CHAR];       //buffer for sending and receiving comms

//data storage
int16_t readBuffer;
float rtc_TEMP;
int batteryValue;

//time settings
unsigned long millisTime;
long currentTime = 0;
long sleepDuration_seconds = 15;
long delayedStart_seconds = 0;
DateTime nextAlarm;
DS3231 rtc; //create RTC object

//SD vars
#define SPI_SPEED SD_SCK_MHZ(50)
char filename[] = "DDMMYYYY.TXT"; 
SdFat sd;
SdFile file;

//ADC vars
Adafruit_ADS1115 ads;
int gain;

/* SETUP
 *  Try to establish coms with GUI.
 *  Initiate the components.
 *  Wait for settings or use default.
 *  Create a text file.
 */

void setup() {
  delay(100); //allow power to stabilize

  //if anything writes to these before started, it will crash.
  Serial.begin(115200);
  Serial.setTimeout(50);
  Wire.begin();

  EEPROM.get(SN_ADDRESS, serialNumber);

  
  /* With power switching between measurements, we need to know what kind of setup() this is.
   *  First, check if the firmware was updated.
   *  Next, check if the GUI connection forced a reset.
   *  If neither, we assume this is a power cycle during deployment and use stored settings.
   */
  bool updatedFirmware = false;
  bool clk_init = true;

  //if new firmware was updated, then take all those settings and time.
  uint32_t storedTime;
  EEPROM.get(UPLOAD_TIME_ADDRESS,storedTime);
  if(uploadDT.unixtime()!=storedTime){
    updatedFirmware = true;
    EEPROM.put(UPLOAD_TIME_ADDRESS,uploadDT.unixtime());
    EEPROM.put(SLEEP_ADDRESS,sleepDuration_seconds);
    Serial.println("Firmware updated");
    clk_init = rtc.begin(); //reset the rtc
    rtc.adjust(uploadDT);
  }
  //otherwise check if the GUI is connected
  //send a startup message and wait a bit for an echo from the gui
  else {
    long tStart = millis();
    while(millis()-tStart<COMMS_WAIT){
      sprintf(messageBuffer,"OPENOBS,%u",serialNumber);
      serialSend(messageBuffer);
      delay(100); //allow time for the gui to process/respond.
      if(serialReceive(&messageBuffer[0])){
        if(strncmp(messageBuffer,"$OPENOBS",8)==0){
          guiConnected = true;
          clk_init = rtc.begin(); //reset the rtc
          break;
        }
      }
    }
  }
  if (guiConnected == false){
    //if no contact from GUI, read last stored value
    EEPROM.get(SLEEP_ADDRESS,sleepDuration_seconds);
  }

  //intialize SD card
  bool sd_init = sd.begin(pChipSelect,SPI_SPEED);
  if(!sd_init) {
    serialSend("SDINIT,0");
  }
  
  //initialize the RTC
  if(!clk_init) {
    serialSend("CLKINIT,0");
  }

  //initialize the ADC
  ads.setGain(GAIN_ONE); //reset gain
  ads.begin(0x48);  // Initialize ads1115
  ads.setDataRate(RATE_ADS1115_860SPS); //set the sampling speed
  ads.readADC_SingleEnded(0); //throw one reading away. Seems to come up bad.
  bool adc_init = ads.readADC_SingleEnded(0) != -1;
  if(!adc_init) {
    serialSend("ADCINIT,0");
  }

  //if we had any errors turn off battery power and stop program.
  //set another alarm to try again- intermittent issues shouldnt end entire deploy.
  //RTC errors likely are fatal though. Will it even wake if RTC fails?
  if(!sd_init | !clk_init | !adc_init){
    //set a new timer
    nextAlarm = DateTime(rtc.now().unixtime() + sleepDuration_seconds);
    rtc.enableAlarm(nextAlarm);
    setBBSQW(); //enable battery-backed alarm
    delay(100); //ensure the alarm is set
    
    rtc.clearAlarm(); //turn off the power
    while(true); //stop program if we have another power source
  }
  
  //if we have established a connection to the java gui, 
  //send a ready message and wait for a settings response.
  //otherwise, use the settings from EEPROM.
  if(guiConnected){
    serialSend("READY");
    //wait indefinitely while user picks settings and clicks 'send' button.
    while(true){
      delay(100); 
      if(serialReceive(&messageBuffer[0])){
        //if we receive a message, start parsing the inividual words
        //hardcoded order of settings string.
        char *tmpbuf;
        tmpbuf = strtok(messageBuffer,",");
        if(strcmp(tmpbuf,"$SET")!=0) break; //somehow received another message.
        tmpbuf = strtok(NULL, ",");
        currentTime = atol(tmpbuf);   
        tmpbuf = strtok(NULL, ",");
        sleepDuration_seconds = atol(tmpbuf);
        tmpbuf = strtok(NULL, "*");
        delayedStart_seconds = atol(tmpbuf);
        
        rtc.adjust(DateTime(currentTime)); //set RTC
        EEPROM.put(SLEEP_ADDRESS,sleepDuration_seconds); //store the new value.
        serialSend("SET,SUCCESS");
        delay(100);
        break;
      }
    }
  }

  //if we received a delayed start, 
  if(delayedStart_seconds>0){
    nextAlarm = DateTime(currentTime + delayedStart_seconds);
    rtc.enableAlarm(nextAlarm);
    setBBSQW(); //enable battery-backed alarm
    serialSend("POWEROFF,1");
    delay(100); //ensure the alarm is set
    rtc.clearAlarm(); //turn off battery
    delay(delayedStart_seconds*1000); //delay program if we have another power source
  }
  
  updateFilename();
  sprintf(messageBuffer,"FILE,OPEN,%s\0",filename);
  serialSend(messageBuffer);
}


/* LOOP
 *  set the next alarm
 *  open the SD card file
 *  read sensor and write to SD
 *  close the SD file.
 *  go to sleep (unless continuous mode)
 */

void loop() {
  //set the next alarm right away. Check it hasn't passed later.
  nextAlarm = DateTime(rtc.now().unixtime() + sleepDuration_seconds);
  rtc.enableAlarm(nextAlarm);
  setBBSQW(); //enable battery-backed alarm

  //read in battery level
  batteryValue = analogRead(A0);
  //convert to Volts and multiply by 2 because of the voltage divider
//  batteryVolts = batteryValue * (2.0 * 5.0 / 1023.0); 
  
  digitalWrite(pVoltageDivider,HIGH);

  millisTime = millis();
  //background measurements
  digitalWrite(pIRED,LOW);
  file.open(filename,O_WRITE | O_APPEND);
  for (int i = 0; i < NUM_BACKGROUND; i++) {
    readBuffer = ads.readADC_SingleEnded(0);
    file.print(rtc.now().unixtime());
    file.print(',');
    file.print(millis() - millisTime);
    file.print(',');
    file.print(readBuffer);
    file.print(',');
    file.print(0);
    file.print(',');
    //only read temperature once per wake cycle.
    if (i==0){
      file.print(rtc.getTemperature());
      file.print(',');
      file.print(batteryValue);
    }
    file.println();
  }

  sprintf(messageBuffer,"%04u,%05d",0,readBuffer);
  serialSend(messageBuffer);

  //illuminated measurements
  digitalWrite(pIRED, HIGH); //turn on the IRED
  for (int i = 0; i < NUM_SAMPLES; i++) {
    gain = 1; 
    readBuffer = ads.readADC_SingleEnded(0);
    file.print(rtc.now().unixtime());
    file.print(',');
    file.print(millis() - millisTime);
    file.print(',');
    file.print(readBuffer);
    file.print(',');
    file.print(gain);
    file.print(',');
    file.println(',');
    
    //occassionally print some data for inspection and to blink the TX lights.
    if ((i+1)%100==0){
      sprintf(messageBuffer,"%04u,%05d",i+1,readBuffer);
      serialSend(messageBuffer);
    }
  }
  digitalWrite(pIRED, LOW); //turn off the IRED
  file.close();
  
  //ensure a 5 second margin for the next alarm before shutting down.
  //if the alarm we set during this wake has already passed, the OBS will never wake up.
  long timeUntilAlarm = nextAlarm.unixtime()-rtc.now().unixtime();
  if(timeUntilAlarm > 5){
    delay(1000); //give the SD card enough time to close the file and reshuffle data.
    serialSend("POWEROFF,1");
    rtc.clearAlarm(); //turn off battery
    //mimic power off when provided USB power
    delay((sleepDuration_seconds - timeUntilAlarm)*1000); 
  }
  
}
