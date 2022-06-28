#include "LoRaWan_APP.h"
#include "Arduino.h"
#include "CubeCell_NeoPixel.h"
#include "GPS_Air530Z.h"
#include "HT_SSD1306Wire.h"
/*
 * set LoraWan_RGB to 1,the RGB active in loraWan
 * RGB red means sending;
 * RGB green means received done;
 */
#ifndef LoraWan_RGB
#define LoraWan_RGB 0
#endif

#define RF_FREQUENCY                                913500000 // Hz
#define RF_FREQUENCY                                913500000 // Hz

#define TX_OUTPUT_POWER                             14        // dBm

#define LORA_BANDWIDTH                              0         // [0: 125 kHz,
                                                              //  1: 250 kHz,
                                                              //  2: 500 kHz,
                                                              //  3: Reserved]
#define LORA_SPREADING_FACTOR                       7         // [SF7..SF12]
#define LORA_CODINGRATE                             1         // [1: 4/5,
                                                              //  2: 4/6,
                                                              //  3: 4/7,
                                                              //  4: 4/8]
#define LORA_PREAMBLE_LENGTH                        8         // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT                         0         // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON                  false
#define LORA_IQ_INVERSION_ON                        false


#define RX_TIMEOUT_VALUE                            1000
#define BUFFER_SIZE                                 29 // Define the payload size here
// the natural log of 1.002 is needed for the altitude encoding
#define LN_1_002 0.00199800266
#define LN_1_08 0.07696104113

CubeCell_NeoPixel pxl(16, RGB, NEO_GRB + NEO_KHZ800);
SSD1306Wire  disp(0x3c, 500000, SDA, SCL, GEOMETRY_128_64, GPIO10 );
Air530ZClass GPS;

char txpacket[BUFFER_SIZE];
char rxpacket[BUFFER_SIZE];
unsigned char foo[4];

static RadioEvents_t RadioEvents;

int txNumber;

int16_t rssi,rxSize;
uint64_t chipID;
uint32_t nextTx = 0;
char sourceId[6];
void  DoubleToString( char *str, double double_num,unsigned int len);
void base91Encode(bool latitude, double value, unsigned char encoded[4]);

void setup() {
  Serial.begin(115200);

  txNumber=0;
  rssi=0;

  pinMode(Vext,OUTPUT);
  digitalWrite(Vext, LOW);
  delay(10);
  
  pxl.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  pxl.clear(); // Set all pixel colors to 'off'

  Radio.Init( &RadioEvents );
  Radio.SetChannel( RF_FREQUENCY );
  Radio.SetTxConfig( MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                                  LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                                  LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                                  true, 0, 0, LORA_IQ_INVERSION_ON, 3000 );
  Radio.SetSyncWord(0x13);
  disp.init();
  disp.clear();
  disp.display();
  
  disp.setTextAlignment(TEXT_ALIGN_CENTER);
  disp.setFont(ArialMT_Plain_16);
  disp.drawString(64, 32-16/2, "GPS init...");
  disp.display();
  
  GPS.begin();

  chipID=getID() & 0xFFFFFFFF;
  sprintf(sourceId, "%02X%04X\r\n",(uint32_t)(chipID>>32),(uint32_t)chipID);
  for ( int i=0; i<BUFFER_SIZE; i++ ) {
    txpacket[i] = 0x00;
  }
}

int fracPart(double val, int n) {
  return (int)((val - (int)(val))*pow(10,n));
}

void loop() {
  uint32_t starttime = millis();
  while (GPS.available() > 0) {
    GPS.encode(GPS.read());
  }
  
  char str[30];
  disp.clear();
  disp.setFont(ArialMT_Plain_10);
  int index = sprintf(str,"%02d-%02d-%02d",GPS.date.year(),GPS.date.day(),GPS.date.month());
  str[index] = 0;
  disp.setTextAlignment(TEXT_ALIGN_LEFT);
  disp.drawString(0, 0, str);
  
  index = sprintf(str,"%02d:%02d:%02d",GPS.time.hour(),GPS.time.minute(),GPS.time.second(),GPS.time.centisecond());
  str[index] = 0;
  disp.drawString(60, 0, str);

  index = sprintf(str,"alt: %d.%d",(int)GPS.altitude.meters(),fracPart(GPS.altitude.meters(),2));
  str[index] = 0;
  disp.drawString(0, 16, str);
   
  index = sprintf(str,"hdop: %d.%d",(int)GPS.hdop.hdop(),fracPart(GPS.hdop.hdop(),2));
  str[index] = 0;
  disp.drawString(0, 32, str); 
 
  index = sprintf(str,"lat :  %d.%d",(int)GPS.location.lat(),fracPart(GPS.location.lat(),4));
  str[index] = 0;
  disp.drawString(60, 16, str);   
  
  index = sprintf(str,"lon:%d.%d",(int)GPS.location.lng(),fracPart(GPS.location.lng(),4));
  str[index] = 0;
  disp.drawString(60, 32, str);

  index = sprintf(str,"%d.%d km/h @ %d°",(int)GPS.speed.kmph(),fracPart(GPS.speed.kmph(),3), GPS.course.deg());
  str[index] = 0;
  disp.drawString(0, 48, str);
  disp.display();

//  turnOnRGB(COLOR_SEND,0); //change rgb color

  if( GPS.location.age() < 2000 ) {
    if ( starttime > nextTx ) {
      disp.drawString(120, 0, "A");
      pxl.setPixelColor(0, pxl.Color(0, 32, 0));
      pxl.show();
      txNumber += 1;
      signed int lat = GPS.location.lat() * 100000;
      signed int lon = GPS.location.lng() * 100000;
      signed int alt = GPS.altitude.feet();
      unsigned int vel = GPS.speed.knots();
      unsigned int cog = (int)round(GPS.course.deg());
      strncpy(txpacket,sourceId,6);
      strncpy(txpacket+6,">APZ001:",8);
      // As this is an APRS format, we must start with an indicator
      // that we are unable to receive messages.
      int i = 14;
      txpacket[i++] = '!';
      txpacket[i++] = '\\';
      base91Encode(true, GPS.location.lat(), foo);
      txpacket[i++] = foo[0];
      txpacket[i++] = foo[1];
      txpacket[i++] = foo[2];
      txpacket[i++] = foo[3];
      base91Encode(false, GPS.location.lng(), foo);
      txpacket[i++] = foo[0];
      txpacket[i++] = foo[1];
      txpacket[i++] = foo[2];
      txpacket[i++] = foo[3];
      txpacket[i++] = '^';
      // alternate sending altitude and course/speed
      if ( txNumber % 2 == 0 ) {
        // we're encoding ALTITUDE into the msg body (vs course/speed)
        int x  = (int)(log(alt) / LN_1_002);
        Serial.printf("altitude:  %d\t",x);
        txpacket[i++]= (char)(x / 91 + 33);
        txpacket[i++]= x % 91 + 33;
        // Set compression type byte to 54 (00110110)
        txpacket[i++] = (char)(54 + 33);
      } else {
        // Encoding COURSE and SPEED into the MSG body
        Serial.printf("%d\t%d\t", cog, vel);
        // ln(0) == -infinity, which doesn't translate well to ascii....
        txpacket[i++] = (char)((cog+1)/4 + 33);
        if ( vel ==0 ) {
          txpacket[i++] = (char)(33);
        } else {
          txpacket[i++] = (char)(log(vel)/LN_1_08 +33);
        }
        // Set compression type byte to 58 (00111010)
        txpacket[i++] = (char)(58 + 33);
      }
      // the Type Byte is encoded as 00110110 (0x36 , 54d)
      Serial.printf("Encoded value:  %s\t\t", txpacket);
      Serial.printf("Lat: %d, lon %d, alt %d, vel: %d, cog: %d, chipId %6x\n", 
                    lat,     lon,    alt,    vel,     cog,     chipID);
      Radio.Send( (uint8_t *)txpacket, i); //send the package out
      nextTx = starttime + 950 + random(250);
    }
  } else {
    disp.drawString(120, 0, "V");
    pxl.setPixelColor(0, pxl.Color(32, 0, 0));
    pxl.show();
  }

}

/**
  * @brief  Double To String
  * @param  str: Array or pointer for storing strings
  * @param  double_num: Number to be converted
  * @param  len: Fractional length to keep
  * @retval None
  */
void  DoubleToString( char *str, double double_num,unsigned int len) { 
  double fractpart, intpart;
  fractpart = modf(double_num, &intpart);
  fractpart = fractpart * (pow(10,len));
  sprintf(str + strlen(str),"%d", (int)(intpart)); //Integer part
  sprintf(str + strlen(str), ".%d", (int)(fractpart)); //Decimal part
}

void base91Encode(bool latitude, double value, unsigned char encoded[4] ) {
  int a = 0;
  if ( latitude ) {
    a = 380926 * ( 90 - value);
  } else {
    a = 190463 * ( 180 + value);
  }
  encoded[0] = (char)(a / pow(91,3) + 33);
  a = a % (int)(pow(91,3));
  encoded[1] = (char)(a / pow(91,2) + 33);
  a = a % (int)(pow(91,2));
  encoded[2] = (char)(a / 91 + 33);
  encoded[3] = (char)(a % 91 +33);
}