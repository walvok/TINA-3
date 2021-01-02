/*
 * This code is made for ESP32 with TFT SPI 2.4 + SD card + GPS (Skylab SKM53)
 * Code is in development.
 */
// Pridani knihoven
#include "Adafruit_GFX.h" //Knihovna pro diplej
#include "Adafruit_ILI9341.h" //Knihovna radice displeje
#include <SPI.h> //Knihovna pro SPI - SD
#include <TinyGPS++.h> //Knihovna pro praci s GPS
#include <mySD.h> //Knihovan pro praci s SD kartou
#include <math.h.> //Matematicka knihovna
#include "ThingSpeak.h" //Knihovna thingspeak serveru
#include <HardwareSerial.h> //Knihovna HW linky
#include <WiFi.h> //Knihovna modulu Wi-FI
// Definice  TFT pinu
#define TFT_DC 12
#define TFT_CS 15
#define TFT_RST 4
#define TFT_MOSI 13
#define TFT_CLK 14
// Definice SD pinu
#define SD_CS 5
#define SD_MOSI  23
#define SD_MISO  19
#define SD_SCK   18

#define BLACK   0x0000
#define WHITE   0xFFFF
// WiFi
#define SECRET_SSID "ssid_wifi_site"    //wifi SSID
#define SECRET_PASS "heslo_wifi_site" //wifi pass
// Server Thingspeak
#define SECRET_CH_ID 1234567      //channel number on server
#define SECRET_WRITE_APIKEY "Zde_vloz_api_klic"   //your API key
// Heart rate sensor
#define analogPin 34
const int zpozdeniMereni = 60;

bool actual = 0, last = 1;
byte state = HIGH;
const double FOURTHPI = PI / 4;
const double deg2rad = PI / 180;
const double rad2deg = 180.0 / PI;
#define equrad = 6377563;
#define squecc = 0.00667054;
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST);
TinyGPSPlus gps; //Inicializace gps
static const int RXPin = 16, TXPin = 17;
HardwareSerial GPS(1); 
static const double CilLat = 12.345678, CilLon = 12.345678; //put your destination coord
float LAT, LON, speed, CAS, SMER, COU;
int AGE, SAT, ALT, tFrec = 0, attempt = 0, bpm;
String TIME = "";
unsigned long temp;
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
byte keyIndex = 0;
WiFiClient  client; 
unsigned long myChannelNumber = SECRET_CH_ID;
const char * myWriteAPIKey = SECRET_WRITE_APIKEY;
String myStatus = "";

void setup() { 
  Serial.begin(9600); 
  tft.begin(); 
  tft.setRotation(0); 
  tft.fillScreen(BLACK); 
  tft.setCursor(0, 0); 
  tft.setTextColor(WHITE, BLACK); 
  tft.setTextSize(2);
  WiFi.mode(WIFI_STA);
  btStop();
  ThingSpeak.begin(client);  // Initialize ThingSpeak
  GPS.begin(9600, SERIAL_8N1, RXPin, TXPin); 
  SD.begin(SD_CS, SD_MOSI, SD_MISO, SD_SCK);  
}

void loop() { //main loop
  if (attempt <= 2) {
    Connect();
  }
  else
  {
    WiFi.mode(WIFI_OFF);
  }
  getGPS();  
  String dataString = ""; //Declaration of variable - for saving values on SD card. Saving is by rows
  double latt = LAT * 1000; //Multiplication of variable for web application
  double lonn = LON * 1000; 
  dataString += String(latt) + "," + String(lonn) + ",";
  int temporary = Dist(LAT, LON, CilLat, CilLon);
  // Nahrani dat na server ThingSpeak
  if (WiFi.status() == WL_CONNECTED) {
    ThingSpeak.setField(1, LAT);
    ThingSpeak.setField(2, LON);
    ThingSpeak.setField(3, SAT);
    ThingSpeak.setField(4, ALT);
    ThingSpeak.setField(5, temporary);
    int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
  }

  static int beatPerMinute = 1;
  tFrec = 0;
  if (detekceTepu(analogPin, zpozdeniMereni)) {
    tFrec = 60000 / beatPerMinute;
    if (tFrec > 50 & tFrec < 200) {
      bpm = tFrec;
    }
    beatPerMinute = 0;
  }
  delay(zpozdeniMereni);
  beatPerMinute += zpozdeniMereni;

  File dataFile = SD.open("datalog.txt", FILE_WRITE);
  // check routine to verify correct open, if yes -> save data to folder
  if (dataFile)
  {
    dataFile.print(dataString);
    dataFile.close();
  }
  if (state == LOW)
  {
    Desk1Data();
  }
  else
    Desk2Data();

  actual = digitalRead(2);
  if (actual == HIGH && last == LOW) {
    if (state == HIGH)
    {
      state = LOW;
      Desk1();
      Desk1Data();
    }
    else
    {
      state = HIGH;
      Desk2();
      Desk2Data();
    }
  }
  last = actual;
}

void Connect()
{
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(ssid, pass);
    delay(5000);
    attempt++;
  }
}

void Desk1()
{
  tft.fillScreen(BLACK);
  tft.setCursor(12, 15);
  tft.print("MGRS");
  tft.setCursor(12, 115);
  tft.print("AGE");
  tft.setCursor(127, 115);
  tft.print("Sat");
  tft.setCursor(12, 215);
  tft.print("Hei");
  tft.setCursor(127, 215);
  tft.print("Time");
  Desk1Data();
}

void Desk1Data()
{
  tft.setCursor(12, 40);
  tft.print(LLtoUTM(LAT, LON));
  tft.setCursor(12, 150);
  tft.print(AGE);
  tft.setCursor(127, 150);
  tft.print(SAT);
  tft.setCursor(12, 250);
  tft.print(ALT);
  tft.setCursor(127, 250);
  tft.print(TIME);
}

void Desk2()
{
  tft.fillScreen(BLACK);
  tft.setCursor(12, 15);
  tft.print("DIST");
  tft.setCursor(12, 115);
  tft.print("TIME");
  tft.setCursor(127, 115);
  tft.print("COURSE");
  tft.setCursor(127, 15);
  tft.print("BPM");
  Desk2Data();
}
void Desk2Data()
{
  temp = Dist(LAT, LON, CilLat, CilLon);
  COU = CourseTo(LAT, LON, CilLat, CilLon);
  tft.setCursor(12, 40);
  tft.print(temp);
  tft.setCursor(12, 150);
  tft.print(TimeTo(temp));
  tft.print(" H");
  tft.setCursor(127, 150);
  tft.print(COU);
  tft.setCursor(127, 40);
  tft.print(bpm);
}

void getGPS() {
  bool newdata = false;
  unsigned long start = millis();
  while (millis() - start < 500)
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
    TIME = gps.time.hour() + 2;
    TIME += ":";
    TIME += gps.time.minute();
    feedgps();
  }
}
bool feedgps() {
  while (GPS.available())
  {
    if (gps.encode(GPS.read()))
      return true;
  }
  return 0;
}

unsigned long Dist(float LAT, float LON, float CilLat, float CilLon)
{
  unsigned long distanceKmToFinish = (unsigned long)TinyGPSPlus::distanceBetween(LAT, LON, CilLat, CilLon) / 1000; // /1000 = in km
  return distanceKmToFinish;
}

double CourseTo(float LAT, float LON, float CilLat, float CilLon)
{
  double courseToFinish = TinyGPSPlus::courseTo(LAT, LON, CilLat, CilLon);
  return courseToFinish;
}

float TimeTo(unsigned long distanceKmToFinish)
{
  float timeto;
  timeto = (distanceKmToFinish * 1.4) / 5; //1.4 magic const, for extension from direct to real distance
  return timeto;
}

String LLtoUTM(const double Lat, const double Long)
{
  float a = 6378137;
  float eccSquared = 0.00669438;
  float k0 = 0.9996;
  double LongOrigin;
  double eccPrimeSquared;
  double N, T, C, A, M;
  double UTMEasting;
  double UTMNorthing;
  float LongTemp = (Long + 180) - int((Long + 180) / 360) * 360 - 180;
  float LatRad = Lat * deg2rad;
  float LongRad = LongTemp * deg2rad;
  float LongOriginRad;
  byte    ZoneNumber;

  ZoneNumber = int((LongTemp + 180) / 6) + 1;
  if ( Lat >= 56.0 && Lat < 64.0 && LongTemp >= 3.0 && LongTemp < 12.0 )
    ZoneNumber = 32;
  if ( Lat >= 72.0 && Lat < 84.0 )
  {
    if (      LongTemp >= 0.0  && LongTemp <  9.0 ) ZoneNumber = 31;
    else if ( LongTemp >= 9.0  && LongTemp < 21.0 ) ZoneNumber = 33;
    else if ( LongTemp >= 21.0 && LongTemp < 33.0 ) ZoneNumber = 35;
    else if ( LongTemp >= 33.0 && LongTemp < 42.0 ) ZoneNumber = 37;
  }
  LongOrigin = (ZoneNumber - 1) * 6 - 180 + 3;
  LongOriginRad = LongOrigin * deg2rad;
  eccPrimeSquared = (eccSquared) / (1 - eccSquared);
  N = a / sqrt(1 - eccSquared * sin(LatRad) * sin(LatRad));
  T = tan(LatRad) * tan(LatRad);
  C = eccPrimeSquared * cos(LatRad) * cos(LatRad);
  A = cos(LatRad) * (LongRad - LongOriginRad);
  M = a * ((1 - eccSquared / 4    - 3 * eccSquared * eccSquared / 64  - 5 * eccSquared * eccSquared * eccSquared / 256) * LatRad
           - (3 * eccSquared / 8 + 3 * eccSquared * eccSquared / 32  + 45 * eccSquared * eccSquared * eccSquared / 1024) * sin(2 * LatRad)
           + (15 * eccSquared * eccSquared / 256 + 45 * eccSquared * eccSquared * eccSquared / 1024) * sin(4 * LatRad)
           - (35 * eccSquared * eccSquared * eccSquared / 3072) * sin(6 * LatRad));

  UTMEasting = (double)(k0 * N * (A + (1 - T + C) * A * A * A / 6
                                  + (5 - 18 * T + T * T + 72 * C - 58 * eccPrimeSquared) * A * A * A * A * A / 120)
                        + 500000.0);
  UTMNorthing = (double)(k0 * (M + N * tan(LatRad) * (A * A / 2 + (5 - T + 9 * C + 4 * C * C) * A * A * A * A / 24
                               + (61 - 58 * T + T * T + 600 * C - 330 * eccPrimeSquared) * A * A * A * A * A * A / 720)));

  if (Lat < 0)
    UTMNorthing += 10000000.0;
  String UTMTwo = MGRSZoneDesignator(UTMEasting, UTMNorthing);
  int corrE = UTMEasting / 100000;
  int UTMEast = UTMEasting - (corrE * 100000);
  int corrN = UTMNorthing / 100000;
  int UTMNorth = UTMNorthing - (corrN * 100000);
  String toUTM = String(ZoneNumber) + " " + UTMLetterDesignator(Lat) + " " + UTMTwo + "\n E:" + UTMEast + "\n N:" + UTMNorth;
  return toUTM;
}

String MGRSZoneDesignator(double UTMEasting, double UTMNorthing)
{
  String e100kLetters[] = {"S", "T", "U", "V", "W", "X", "Y", "Z"};
  String n100kLetters[] = {"A", "B", "C", "D", "E", "F", "G", "H", "J", "K", "L", "M", "N", "P", "Q", "R", "S", "T", "U", "V"};
  const int col = floor(UTMEasting / 100000); //Cutting first number. Upside is more info
  const int row = int(floor(UTMNorthing / 100000)) % 20; //Cutting first two numbers. Upside is more info
  String ZoneDesignator = e100kLetters[col - 1];
  ZoneDesignator += n100kLetters[row];
  return ZoneDesignator;
}

char UTMLetterDesignator(double Lat)
{
  char LetterDesignator;
  if ((84 >= Lat) && (Lat >= 72)) LetterDesignator = 'X';
  else if ((72 > Lat) && (Lat >= 64)) LetterDesignator = 'W';
  else if ((64 > Lat) && (Lat >= 56)) LetterDesignator = 'V';
  else if ((56 > Lat) && (Lat >= 48)) LetterDesignator = 'U';
  else if ((48 > Lat) && (Lat >= 40)) LetterDesignator = 'T';
  else if ((40 > Lat) && (Lat >= 32)) LetterDesignator = 'S';
  else if ((32 > Lat) && (Lat >= 24)) LetterDesignator = 'R';
  else if ((24 > Lat) && (Lat >= 16)) LetterDesignator = 'Q';
  else if ((16 > Lat) && (Lat >= 8)) LetterDesignator = 'P';
  else if (( 8 > Lat) && (Lat >= 0)) LetterDesignator = 'N';
  else if (( 0 > Lat) && (Lat >= -8)) LetterDesignator = 'M';
  else if ((-8 > Lat) && (Lat >= -16)) LetterDesignator = 'L';
  else if ((-16 > Lat) && (Lat >= -24)) LetterDesignator = 'K';
  else if ((-24 > Lat) && (Lat >= -32)) LetterDesignator = 'J';
  else if ((-32 > Lat) && (Lat >= -40)) LetterDesignator = 'H';
  else if ((-40 > Lat) && (Lat >= -48)) LetterDesignator = 'G';
  else if ((-48 > Lat) && (Lat >= -56)) LetterDesignator = 'F';
  else if ((-56 > Lat) && (Lat >= -64)) LetterDesignator = 'E';
  else if ((-64 > Lat) && (Lat >= -72)) LetterDesignator = 'D';
  else if ((-72 > Lat) && (Lat >= -80)) LetterDesignator = 'C';
  else LetterDesignator = 'Z';
  return LetterDesignator;
}

bool detekceTepu(int senzorPin, int zpozdeni) { //detection of heart beat
  static int maxHodnota = 0;
  static bool SpickovaHodnota = false;
  int analogHodnota;
  bool vysledek = false;
  analogHodnota = analogRead(senzorPin);
  analogHodnota *= (1000 / zpozdeni);
  if (analogHodnota * 4L < maxHodnota) {
    maxHodnota = analogHodnota * 0.8;
  }
  if (analogHodnota > maxHodnota - (1000 / zpozdeni)) {
    if (analogHodnota > maxHodnota) {
      maxHodnota = analogHodnota;
    }
    if (SpickovaHodnota == false) {
      vysledek = true;
    }
    SpickovaHodnota = true;
  } else if (analogHodnota < maxHodnota - (3000 / zpozdeni)) {
    SpickovaHodnota = false;
    maxHodnota -= (1000 / zpozdeni);
  }
  return vysledek;
}
