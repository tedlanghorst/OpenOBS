//Check if the daily file exists already. If not, create one and write headers.
void updateFilename(){
  DateTime now = rtc.now();
  snprintf(filename, 13, "%02u%02u%04u.TXT", now.date(), now.month(), now.year());

  SdFile::dateTimeCallback(dateTime_callback);
  //if we create a new file with this name, set header
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
