#include <EEPROM.h>

#define SN_ADDRESS 500

uint16_t tmp_SN;
uint16_t serialNumber;

void setup() {
  Serial.begin(115200);

  EEPROM.get(SN_ADDRESS,serialNumber);

  Serial.println("===== Current Settings =====");
  Serial.print("Serial Number: ");
  Serial.println(serialNumber);
  Serial.println("============================");

  Serial.println();
  Serial.println("== Enter a new SN or quit ==");
}

void loop() {
  while(Serial.available()==0){}; //wait for input
  tmp_SN = Serial.parseInt();

  if (tmp_SN==0){return;}

  //put and get just to make sure EEPROM is working.
  EEPROM.put(SN_ADDRESS,tmp_SN);
  EEPROM.get(SN_ADDRESS,serialNumber); 

  Serial.println("========== New SN ==========");
  Serial.print("Serial Number: ");
  Serial.println(serialNumber);
  Serial.print("Stored at address: ");
  Serial.println(SN_ADDRESS);
  Serial.println("============================");
  Serial.println();
  Serial.println("== Enter a new SN or quit ==");
}
