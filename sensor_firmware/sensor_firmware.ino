/*
  Test strings for serial monitor
  $OPENOBS*4A
  $SET,1616605996,900*7E
*/

/*TODO
  - Find next wake time at begining of sampling, not the end. It should occur every x seconds, not x seconds + sampling time.
    Need to make sure the time has not already passed before going to sleep.
  - write/implement error_shutdown() function.
  - make sure sleepDuration is correctly set if loading new code.
  --> store code upload time in a specific address
  --> if the upload time is different than stored value, then new firmware was loaded.
*/
#include <Wire.h>               //standard library
#include <SPI.h>                //standard library
#include <EEPROM.h>             //standard library
#include <Adafruit_ADS1015.h>   // https://github.com/adafruit/Adafruit_ADS1X15
#include <DS3231.h>             // https://github.com/kinasmith/DS3231
#include <SdFat.h>              // https://github.com/greiman/SdFat //uses 908 bytes of memory

/*
 *  CONFIGURATION SETTINGS
 */
 
//firmware data
const char codebuild[] PROGMEM = __FILE__;  // loads the compiled source code directory & filename into a varaible
const DateTime uploadDT = DateTime((__DATE__),(__TIME__));
const char contactInfo[] PROGMEM = "if found, contact efe@unc.edu"; 
const char dataColumnLabels[] PROGMEM = "time,R0,Rv,gain,temp"; //could shift to local variable in filenameUpdate() if we need more progmem
uint16_t serialNumber;

//sampling constants
const uint16_t NUM_SAMPLES = 1000;

//connected pins
#define pVoltageDivider 4    //voltage divider
#define pIRED A3             //IR emitter
#define pAlarmInterrupt 2    //alarm interrupt from RTC
#define pChipSelect 10       //chip select pin for SD card

//EEPROM addresses
#define SLEEP_ADDRESS 0
#define SN_ADDRESS 500
#define UPLOAD_TIME_ADDRESS 502

//communications vars
const uint16_t COMMS_WAIT = 500;   //ms delay to try gui connection
const int MAX_CHAR = 40;            //max character in NMEA-style string
char messageBuffer[MAX_CHAR];       //buffer for sending and receiving comms

//data storage
int16_t readBuffer;
float rtc_TEMP;

//time settings
long currentTime = 0;
long sleepDuration_seconds = 15;
long delayedStart_seconds = 0;
DateTime nextAlarm;
DS3231 rtc; //create RTC object

//SD vars
#define SPI_SPEED SD_SCK_MHZ(50)
char filename[] = "DDMMYYYY.TXT"; 
//SdFs sd;
//FsFile file;
SdFat sd;
SdFile file;


//ADC vars
Adafruit_ADS1115 ads1115(0x48); //address for ADDR connect to GND


/* SETUP
 *  try to establish coms with GUI
 *  initiate components
 *  wait for settings or use default
 *  create text file
 */

void setup() {
  delay(100); //allow power to stabilize

  //if anything writes to these before started, it will crash.
  Serial.begin(115200);
  Serial.setTimeout(50);
  Wire.begin();

  EEPROM.get(SN_ADDRESS, serialNumber);

  
  /* With power  switching between measurements, we need to know what kind of setup() this is.
   *  First, check if the firmware upload time is different than the stored time.
   *  Next, check if the GUI connection forced a reset.
   *  If neither, we assume this is a power cycle during deployment and use stored settings.
   */
  bool updatedFirmware = false;
  bool guiConnected = false;
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
  ads1115.begin();  // Initialize ads1115
  ads1115.setGain(GAIN_ONE); //reset gain
  ads1115.setSPS(ADS1115_DR_860SPS); //set the sampling speed
  ads1115.readADC_SingleEnded(0); //throw one reading away. Seems to come up bad.
  bool adc_init = ads1115.readADC_SingleEnded(0) != -1;
  if(!adc_init) {
    serialSend("ADCINIT,0");
  }

  //turn off battery power and stop program.
  if(!sd_init | !clk_init | !adc_init){
    rtc.clearAlarm();
    while(true);
  }
  
  //if we have established a connection to the java gui, 
  //send a ready message and wait for a settings response.
  //otherwise, use the settings from EEPROM.
  if(guiConnected){
    serialSend("READY");
    //wait while user picks settings and clicks 'send' button.
    while(true){
      delay(100); 
      if(serialReceive(&messageBuffer[0])){
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
  updateFilename();
  sprintf(messageBuffer,"FILE,OPEN,%s\0",filename);
  serialSend(messageBuffer);

}


/* LOOP
 *  set the next alarm
 *  open the SD card file
 *  read ADC and write to SD
 *  close the SD file.
 *  go to sleep (unless continuous mode)
 */

void loop() {
  //set the next alarm right away. Check it hasn't passed later.
  DateTime wakeTime = rtc.now(); //get the current time
  nextAlarm = DateTime(wakeTime.unixtime() + sleepDuration_seconds);
  rtc.enableAlarm(nextAlarm);
  setBBSQW(); //enable battery-backed alarm
  
  digitalWrite(pIRED, HIGH);
  digitalWrite(pVoltageDivider,HIGH);
  file.open(filename,O_WRITE | O_APPEND);
  for (int i = 0; i < NUM_SAMPLES; i++) {
    readBuffer = ads1115.readADC_SingleEnded(0);

    int gain = 1;
    
    file.print(rtc.now().unixtime());
    file.print(',');
    file.print(readBuffer);
    file.print(',');
    file.print(0);
//    file.print(ads1115.readADC_SingleEnded(gain));
    file.print(',');
    file.print(gain);
    file.print(',');
    if (i==0){
      file.println(rtc.getTemperature());
    }
    else {
      file.println();
    }
    
    // print some sample data while gui connected 
    if ((i+1)%100==0 && guiConnected){
      sprintf(messageBuffer,"%04u,%05u",i+1,readBuffer);
      serialSend(messageBuffer);
    }
  }
  file.close();

  //ensure a 5 second margin for the next alarm before shutting down.
  //if the alarm we set during this wake has already passed, the OBS will never wake up.
  long alarmDelta = rtc.now().unixtime()-wakeTime.unixtime();
  if(alarmDelta < (sleepDuration_seconds-5)){
    serialSend("POWEROFF,1");
    rtc.clearAlarm();
    delay((sleepDuration_seconds - alarmDelta)*1000); //mimic power off.
  }
}
