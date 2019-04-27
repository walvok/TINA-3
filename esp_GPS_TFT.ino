/*
 * This code is made for ESP32 with TFT SPI 2.4 + SD card + GPS (Skylab SKM53)
 * Code is in development.
 */
// Includes of libraries
#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include <TinyGPS++.h>
#include <mySD.h>

// Definitons of TFT pins
#define TFT_DC 12 //4
#define TFT_CS 15 //15
#define TFT_RST 4 //2
//#define TFT_MISO 19
#define TFT_MOSI 13      //23     
#define TFT_CLK 14 //18
// Definitions of pins of SD pins
#define SD_CS 5 //13
#define SD_MOSI  23
#define SD_MISO  19
#define SD_SCK   18

// Colours
#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define MAGENTA 0xF81F
#define WHITE   0xFFFF

//GPS set-up
static const int RXPin = 16, TXPin = 17;
HardwareSerial GPS(1);
TinyGPSPlus gps;
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST);//, TFT_MISO

// Declaration of variables
float LAT, LON, ALT, SAT;
int AGE;
String TIME = "";

void setup() {
  Serial.begin(115200); //Serial is for testing purpose, not needed. 
  GPS.begin(9600, SERIAL_8N1, RXPin, TXPin); //Initialization of GPS module
  Serial.print("Initializing SD card..."); //Serial is for testing purpose, not needed. 
  if (!SD.begin(SD_CS, SD_MOSI, SD_MISO, SD_SCK)) {//Initialization of SD card
    Serial.println("SD card failed!"); //Serial is for testing purpose, not needed. 
    return;
  }
  Serial.println("SD card initialised!"); //Serial is for testing purpose, not needed. 
  tft.begin(); //Initialization of TFT
  tft.setRotation(0); //Setting rotation to zero, it is possible to change by 90 degrees
  tft.fillScreen(BLACK); //Setting black background
  tft.setCursor(0, 0); //Setting position of cursor to 0,0
  tft.setTextColor(WHITE,RED); //Setting text and background color
  tft.setTextSize(2);
  tft.drawRect(15, 10, 100, 90, RED); //Drawing top left rectangle
  tft.fillRect(15,10,100,90,RED);
  tft.setCursor(20, 15);
  tft.print("Lat");
  tft.drawRect(130, 10, 100, 90, RED); //Drawing top right rectangle
  tft.fillRect(130,10,100,90,RED);
  tft.setCursor(135, 15);
  tft.print("Lon");
  tft.setTextColor(WHITE,BLUE);
  tft.drawRect(15, 110, 100, 90, BLUE); //Drawing middle left rectangle
  tft.fillRect(15,110,100,90,BLUE);
  tft.setCursor(20, 115);
  tft.print("Age");
  tft.drawRect(130, 110, 100, 90, BLUE); //Drawing middle right rectangle
  tft.fillRect(130,110,100,90,BLUE);
  tft.setCursor(135, 115);
  tft.print("Sat");
  tft.setTextColor(WHITE,MAGENTA);
  tft.drawRect(15, 210, 100, 90, MAGENTA); //Drawing lower left rectangle
  tft.fillRect(15,210,100,90,MAGENTA);
  tft.setCursor(20, 215);
  tft.print("Height");
  tft.drawRect(130, 210, 100, 90, MAGENTA); //Drawing lower right rectangle
  tft.fillRect(130,210,100,90,MAGENTA);
  tft.setCursor(135, 215);
  tft.print("Time");
}


void loop(void) {
  getGPS();  //Calling function getGPS - getting values from GPS
  tft.setTextColor(WHITE,RED);
  tft.setCursor(18,50);
  tft.print(LAT,5);
  tft.setCursor(133,50);
  tft.print(LON,5);
  tft.setTextColor(WHITE,BLUE); 
  tft.setCursor(18,150);
  tft.print(AGE);
  tft.setCursor(133,150);
  tft.print(SAT);
  tft.setTextColor(WHITE,MAGENTA); 
  tft.setCursor(18,250);
  tft.print(ALT);
  tft.setCursor(133,250);
  tft.print(TIME);
  //Saving data from GPS to file datalog.txt on SD card
  //If datalog.txt isnt created, it will be created
  String dataString = ""; //Declaration of variable - for saving values on SD card. Saving is by rows
  dataString += "ASCII";
  dataString += ",";
  dataString += "City";
  dataString += ",";
  double latt = LAT*1000; //Multiplication of variable for web application
  dataString += String(latt);
  double lonn = LON*1000;
  dataString += ",";
  dataString += String(lonn);
  File dataFile = SD.open("datalog.txt", FILE_WRITE);
  if (dataFile) 
    {
      dataFile.println(dataString);
      dataFile.close();
      Serial.println(dataString);
    }  
    else 
    {
      Serial.println("error opening datalog.txt");
    } 
}

void getGPS(){
  bool newdata = false;
  unsigned long start = millis();
  while (millis() - start <1000)
    {
      if (feedgps ())
        {
          newdata = true;
        }
    }
  if (newdata)
    {
      LAT = gps.location.lat();
      LON = gps.location.lng();
      SAT = gps.satellites.value();
      ALT = gps.altitude.meters();      
      AGE = gps.location.age();
      TIME = gps.time.hour()+2; //Value of constant depends on time
      TIME += ":";
      TIME += gps.time.minute();
      feedgps(); 
    }
}

bool feedgps(){
  while (GPS.available())
  {
    if (gps.encode(GPS.read()))
    return true;
  }
  return 0;
}
