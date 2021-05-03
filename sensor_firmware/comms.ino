bool serialReceive(char *sentence, int maxChar){
  //look for a $, initiating NMEA-style string.
  int idx = 0;
  while(Serial.available()>0){
    if(Serial.read() == '$'){
      *sentence++ = '$';
      break;
    } else if(idx++ > maxChar){
      //read a bunch of junk. return control to loop().
      return false;
    }
  }

  //collect NMEA-style string
  idx = 1; //if we get here, $ is at idx 0.
  int idxChk = maxChar-2;
  while(Serial.available()>0 && idx<=idxChk+2){
    char incoming = Serial.read();
    if(incoming=='*'){
      idxChk = idx; 
    }
    *sentence++ = incoming;
    idx++;
  }
  *sentence = '\0'; //terminate
  
  if(testChecksum((sentence-idx))){
    return true;
  }
  return false;

 }

void serialSend(char sentence[]){
  char checksum[2];
  const char* p = generateChecksum(&sentence[0], checksum);
//  checksum[2] = '\0';
  
  Serial.print('$');
  Serial.print(sentence);
  Serial.print('*');
  Serial.print(checksum[0]);
  Serial.println(checksum[1]);

  Serial.flush();
}

const char* generateChecksum(const char* s, char* checksum){
  uint8_t c = 0;
  // Initial $ is omitted from checksum, if present ignore it.
  if (*s == '$')
    ++s;

  while (*s != '\0' && *s != '*')
    c ^= *s++;
    
  if (checksum) {
    checksum[0] = toHex(c / 16);
    checksum[1] = toHex(c % 16);
//    checksum[2] = '\0';
  }
  return s;
}

bool testChecksum(const char* s)
{
  char checksum[2];
  const char* p = generateChecksum(s, checksum);

  return *p == '*' && p[1] == checksum[0] && p[2] == checksum[1];
}

static char toHex(uint8_t nibble)
{
  if (nibble >= 10)
    return nibble + 'A' - 10;
  else
    return nibble + '0';

}
