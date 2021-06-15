/*function reads in the available serial data, parses for an NMEA-style sentence, 
   * and verifies the checksum. The function returns the result of the checksum and 
   * the sentence is stored in the pointer argument for later parsing.
  */
bool serialReceive(char *sentence){
  //look for a $, initiating NMEA-style string.
  int idx = 0;
  while(Serial.available()>0){
    if(Serial.read() == '$'){
      *sentence++ = '$';
      break;
    } else if(idx++ > MAX_CHAR){
      //read a bunch of junk. return control to loop().
      return false;
    }
  }

  //collect NMEA-style string
  idx = 1; //if we get here, $ is at idx 0.
  int idxChk = MAX_CHAR-2;
  while(Serial.available()>0 && idx<=idxChk+2){
    char incoming = Serial.read();
    if(incoming=='*'){
      idxChk = idx; 
    }
    *sentence++ = incoming;
    idx++;
  }
  *sentence = '\0'; //terminate

  //returns true if we received a valid sentence
  return testChecksum((sentence-idx));
 }

//takes a sentence, formats it in NMEA-style, and prints to serial.
void serialSend(char sentence[]){
  char checksum[2];
  const char* p = generateChecksum(&sentence[0], checksum);
  
  Serial.print('$');
  Serial.print(sentence);
  Serial.print('*');
  Serial.print(checksum[0]);
  Serial.println(checksum[1]); 
  //why did I print each checksum char separately ? 
  //can't remember why this was needed.

  Serial.flush();
}

//calculates and returns the 2 char XOR checksum from sentence
const char* generateChecksum(const char* s, char* checksum){
  uint8_t c = 0;
  // Initial $ is omitted from checksum, if present ignore it.
  if (*s == '$')
    ++s;

  //iterate through with bitwise XOR
  while (*s != '\0' && *s != '*')
    c ^= *s++;

  if (checksum) {
    checksum[0] = toHex(c / 16);
    checksum[1] = toHex(c % 16);
  }
  return s;
}

//returns true if the checksum at end of sentence matches a calculated one.
bool testChecksum(const char* s){
  char checksum[2];
  const char* p = generateChecksum(s, checksum);
  return *p == '*' && p[1] == checksum[0] && p[2] == checksum[1];
}

static char toHex(uint8_t nibble){
  if (nibble >= 10)
    return nibble + 'A' - 10;
  else
    return nibble + '0';
}
