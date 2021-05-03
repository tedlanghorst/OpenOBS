//Check if the daily file exists already. If not, create one and write headers.
void updateFilename(){
  DateTime now = rtc.now();
  snprintf(filename, 13, "%02u%02u%04u.TXT", now.date(), now.month(), now.year());

  SdFile::dateTimeCallback(dateTime_callback);
  if (file.open(filename, O_CREAT | O_EXCL | O_WRITE)) {
    
    file.println((__FlashStringHelper*)contactInfo);
    file.print(F("Running: "));
    file.println((__FlashStringHelper*)codebuild); // writes the entire path + filename to the start of the data file
    file.print("OpenOBS SN:");
    file.println(serialNumber);
    file.println();
    file.println((__FlashStringHelper*)dataColumnLabels);
    //Note: SD cards can continue drawing system power for up to 1 second after file close command
//    file.close();
//    delay(1000);
    }
}


//callback for SD file creation date.
void dateTime_callback(uint16_t* date, uint16_t* time) {
  DateTime now = rtc.now();
  // return date using FAT_DATE macro to format fields
  *date = FAT_DATE(now.year(), now.month(), now.date());
  // return time using FAT_TIME macro to format fields
  *time = FAT_TIME(now.hour(), now.minute(), now.second());
}


/*
   interrupt functions
*/
//enable alarm on battery power. Normally disabled
void setBBSQW(){
  uint8_t ctReg = rtc.readRegister(DS3231_CONTROL_REG);
  ctReg |= 0b01000000;
  rtc.writeRegister(DS3231_CONTROL_REG,ctReg); 
}

/*
//Called by the interrupt when it is triggered by the RTC
//continues loop() after exiting. Keep execution very short.
void interruptRoutine() {
  //first it Disables the interrupt so it doesn't get retriggered
  detachInterrupt(digitalPinToInterrupt(pAlarmInterrupt));
  Serial.println("awake");
}

//Puts the MCU into power saving sleep mode and sets the wake time
void enterSleep(DateTime& dt) { //argument is Wake Time as a DateTime object
  rtc.clearAlarm(); //resets the alarm interrupt status on the RTC
  rtc.enableAlarm(dt); //Sets the alarm on the RTC to the specified time (using the DateTime Object passed in)
  attachInterrupt(digitalPinToInterrupt(pAlarmInterrupt), interruptRoutine, FALLING);
  digitalWrite(pIRED, LOW);
  digitalWrite(pVoltageDivider,LOW);
  Serial.println("power down");
  delay(100); //wait for a moment for everything to complete
  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF); //power down everything until the alarm fires
}


*/
