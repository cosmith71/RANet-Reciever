#include <Wire.h>
#include <avr/eeprom.h>

#define BLUE_LED    9
#define WHITE_LED   10

#define BLUE_INTENSITY   255
#define WHITE_INTENSITY  255

#define RANET_MAX_SIZE  64
#define DISCONNECT_TIMEOUT  2000

#define LastFallback0  100 // Memory location for fallback storage

#define RANet_Down    0
#define RANet_OK      1

byte buffer_index;
byte buffer[128];
char buf[3];
byte bufint, bufsize;
byte RANetData[RANET_MAX_SIZE];
byte RANetCRC;
byte BlueChannel=0;
byte WhiteChannel=0;
byte RANet_Status=RANet_Down;
boolean cable_present=false;

unsigned long lastmillis=millis();
unsigned long lastcablecheck=millis();

void setup()
{
  pinMode(BLUE_LED,OUTPUT);
  pinMode(WHITE_LED,OUTPUT);
  Serial.begin(57600);
  Wire.onReceive(NULL);
  Wire.onRequest(NULL);
  Wire.begin();
  for (int a=0;a<RANET_MAX_SIZE; a++) // Clear array
    RANetData[a]=0; 
  Wire.beginTransmission(0x68);
  Wire.write(0);
  int a=Wire.endTransmission();
  cable_present=(a==0);
  // setup PCA9685 for data receive
  // we need this to make sure it will work if connected ofter controller is booted, so we need to send it all the time.
  Wire.beginTransmission(0x40);
  Wire.write(0);
  Wire.write(0xa1);
  Wire.endTransmission();
}

void loop()
{
  if (cable_present)
  {
    BlueChannel=100;
    WhiteChannel=0;
    analogWrite(WHITE_LED,WHITE_INTENSITY*WhiteChannel/100);
    analogWrite(BLUE_LED,BLUE_INTENSITY*BlueChannel/100);
  }
  else  
  {
    BlueChannel=0;
    WhiteChannel=0;
    while(Serial.available())
    {
      UpdateWhiteChannel();
      char c = Serial.read(); // Read each incoming byte
      buffer[buffer_index]=c; // store in the buffer array
      if (c==10) // if line feed we analyze the payload
      {
        if (buffer_index>25) // only need to analyse if buffer_index is greater than 25, otherwise the payload is broken or corrupt
          if (buffer_index==buffer[1]) // check if payload matches the length the controller sent
          {
            UpdateWhiteChannel();
            RANetCRC=0;
            for (int a=0; a<(buffer_index-2); a++) // calculate CRC
              RANetCRC+=buffer[a];
            UpdateWhiteChannel();
            if (RANetCRC==buffer[buffer_index-2]) // if CRC matches
            {
              UpdateWhiteChannel();
              for (int a=0; a<(buffer_index-2); a++) // Copy buffer to RANetData
                RANetData[a]=buffer[a];
              UpdateWhiteChannel();
              lastmillis=millis();
  //            Serial.print(millis());
  //            Serial.print("\t");
  //            Serial.println(RANetData[2]);
              for (int a=0;a<8;a++)
              {
                if (eeprom_read_byte((unsigned char *) LastFallback0+a)!=RANetData[10+a])
                {
                  eeprom_write_byte((unsigned char *) LastFallback0+a, RANetData[10+a]);
                }
                Wire.beginTransmission(0x38+a);
                Wire.write(~RANetData[2+a]);
                Wire.endTransmission();
              }
              for (int a=0;a<6;a++)
              {
                int newdata=(int)(RANetData[18+a]*40.95);
                Wire.beginTransmission(0x40);
                Wire.write(0x8+(4*a));
                Wire.write(newdata&0xff);
                Wire.write(newdata>>8);
                Wire.endTransmission();
              }
              UpdateWhiteChannel();
              RANet_Status=RANet_OK;
            }
          }
        buffer_index=255; // reset buffer index
      }
      UpdateWhiteChannel();
      if (buffer_index++>=128) buffer_index=0; // increment index of buffer array. reset index if >=128
    }
    if (millis()-lastmillis>DISCONNECT_TIMEOUT)
    {
      lastmillis=millis();
  //    Serial.println("Disconnected");
  //    Serial.print(millis());
  //    Serial.print("\t");
  //    Serial.println(RANetData[10]);
      for (int a=0;a<8;a++)
      {
        Wire.beginTransmission(0x38+a);
        Wire.write(~eeprom_read_byte((unsigned char *) LastFallback0+a));
        Wire.endTransmission();
      }
      RANet_Status=RANet_Down;
    }
    if (RANet_Status==RANet_Down)
    {
        BlueChannel=0;
        WhiteChannel=millis()%2000<1000?0:100;
        analogWrite(WHITE_LED,WHITE_INTENSITY*WhiteChannel/100);
        analogWrite(BLUE_LED,BLUE_INTENSITY*BlueChannel/100);
    }
  }
}

void UpdateWhiteChannel()
{
  WhiteChannel=sin(radians((millis()%7200)/40))*255;
  BlueChannel=255-(sin(radians((millis()%7200)/40))*255);
  analogWrite(WHITE_LED,WhiteChannel);
  analogWrite(BLUE_LED,BlueChannel);
}
//00301200000000000000050000000000000000000000000047
//01301200000000000000050000000000000000000000000048
//02301200000000000000050000000000000000000000000049
//0330120000000000000005000000000000000000000000004a



