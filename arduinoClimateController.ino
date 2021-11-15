/* Steve.a.mccluskey@gmail.com
 * 
 * Controls up to 5 relays by comparing the temp from a oneWire sensor and a AM2315 temp/humidity sensor.
 * All parameters are user controllable on LCD screen. 
 * 
 * Hardware used:
 * Arudino Mega2560.
 * Dallas OneWire DS18B20 waterproof digital temp sensor.
 * AM2315 I2C digital temperature/humidity sensor.
 * Adafruit PCF8523 I2C real time clock chip.
 * Adafruit RGB LCD shield. 
 * Arduino Uno proto shield.
 * Velleman VMA400 4 channel relay board.
 * 
 *   
 *   Pin layout as follows:
 * 0  : Hardware serial RX.
 * 1  : Hardware serial TX.
 * 2  : Relay pin 1.
 * 3 ~: Relay pin 2.
 * 4  : Aux relay pin.
 * 5 ~: Relay pin 3.
 * 6 ~: Relay Pin 4.
 * 7  : Onewire bus.
 * 8  : 
 * 9 ~: 
 * 10~: 
 * 11~: 
 * 12 : 
 * 13 : 
 * 14 - 19 : N/C.
 * 20 : SDA.
 * 21 : SCL.
 * 22 - 49 : N/C.
 * 50 : SPI MISO.
 * 51 : SPI MOSI.
 * 52 : SPI SCK.
 * 53 : SPI SS.
 *
 * A0 - A15: N/C.
 *
*/



#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <Adafruit_RGBLCDShield.h>
#include <EEPROM.h>
#include <Adafruit_AM2315.h>
#include "RTClib.h"
#include "Adafruit_SHT31.h"

#define debug //comment out to disable serial code.


//EEPROM locations:
#define tempSetPointEEP      10    // Temperature set point.
#define humSetPointEEP       11    // Humidity set point.
#define menuEEP              12    // Current menu.
#define fanDurationEEP       13    // Fan duration.
#define fanIntervalEEP       14    // Fan interval.
#define lightOnTimeEEP       15    // Light on time.
#define lightOffTimeEEP      16    // Light off time.
#define heatEnabledEEP       17    // Heat enable/disable.
#define lightEnabledEEP      18    // Light enable/disable.
#define humidifierEnabledEEP 19    // Humidifier enable/disable.
#define fanEnabledEEP        20    // Fan enable/disable.

#define heaterInterval       10000 // How long heat must be on or off minimum.
#define sensorReadInterval   3000  // How often to read sensors.
#define humidifierInterval   10000 // How long humidifier must be on or off minimum.
#define sht31heaterInterval  180000 // Humiditity/temp sensor internal heater element timer.
#define sht31heaterDuration  5000   // How long to keep sensor heater on.

#define tempCalibration      -1.6 // For calibrating Humidity/Temp sensor to oneWire sensor.


// Pin declarations:
#define ONE_WIRE_BUS         7

#define relayPinAux          4 //on 3 pin connector

//4 channel relay bank on 6 pin connector:
#define relayPin1            2
#define relayPin2            3
#define relayPin3            5
#define relayPin4            6

#define heatRelay            relayPin1
#define lightRelay           relayPin2
#define fanRelay             relayPin3
#define humidifierRelay      relayPin4

#define menuLimit            9 // Sets max number of menu screens.

// LCD screen colors:
#define RED    0x1
#define YELLOW 0x3
#define GREEN  0x2
#define TEAL   0x6
#define BLUE   0x4
#define VIOLET 0x5
#define WHITE  0x7

OneWire oneWire(ONE_WIRE_BUS);

// Hardware object declarations:
Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();
Adafruit_SHT31 sht31      = Adafruit_SHT31();
RTC_PCF8523    rtc;

bool heatOn               = false;
bool fanOn                = false;
bool humidifierOn         = false;
bool sht31heater          = false;

// Declare booleans and get their saved values at startup:
bool heatEnabled          = EEPROM.read(heatEnabledEEP);
bool fanEnabled           = EEPROM.read(fanEnabledEEP);
bool lightEnabled         = EEPROM.read(fanEnabledEEP);
bool humidifierEnabled    = EEPROM.read(humidifierEnabledEEP);

// Declare some global variables and get their saved values at startup:
byte     sensor1[8];           // Array to store OneWire device's address.
int8_t   oneWireTemp      = 0; //temp value of oneWire sensor.
float    sht31temp        = 0;
uint8_t  humidity         = 0;
int8_t   menu             = EEPROM.read(menuEEP);
uint8_t  setPointTemp     = EEPROM.read(tempSetPointEEP);
int8_t   setPointHumidity = EEPROM.read(humSetPointEEP);
int8_t   fanDuration      = EEPROM.read(fanDurationEEP);
uint8_t  fanInterval      = EEPROM.read(fanIntervalEEP);
uint8_t  lightOnTime      = EEPROM.read(lightOnTimeEEP);
uint8_t  lightOffTime     = EEPROM.read(lightOffTimeEEP);
uint8_t  hour;
uint8_t  minute;
uint8_t  second;

// Duration timer variables:
unsigned long timeNow;
unsigned long heatTimeLast;  
unsigned long sensorTimeLast;
unsigned long fanTimeOn;
unsigned long fanTimer;
unsigned long humidifierTimeLast;
unsigned long sht31heaterTimer;
unsigned long sht31heaterOnTime;

//float temperature, humidityReading; //from am2315

void setup() {
  #ifdef debug
    Serial.begin(19200);
  #endif
  pinMode(heatRelay,       OUTPUT);
  pinMode(lightRelay,      OUTPUT);
  pinMode(fanRelay,        OUTPUT);
  pinMode(humidifierRelay, OUTPUT);
  pinMode(relayPinAux,     OUTPUT);
  
  lcd.begin(16, 2);
  lcd.print("Myco Control!");
  delay(1000);
  lcd.clear();
  oneWire.search(sensor1);

  if (!sht31.begin(0x44)) {
    lcd.print(F("SHT31 not found."));
    delay(1000);
    lcd.clear();
  } // end if.
} // end setup().

void loop() {
  timeNow      = millis();
  DateTime now = rtc.now();
  hour         = now.hour();
  minute       = now.minute();
  second       = now.second();

  // Check to see if any buttons have been pressed and deal with them accordingly:
  uint8_t buttons = lcd.readButtons();
  if (buttons) {
   switch(buttons) {
    case 1: // select:
      switch(menu) {
        case 1: // select button in menu 1 currently unassigned.
        break; // end case 1.

        case 2: // enable/disable heater:
          heatEnabled = !heatEnabled;
          EEPROM.write(heatEnabledEEP, heatEnabled);
        break; // end case 2.

        case 3: // enable/disable humidifier:
          humidifierEnabled = !humidifierEnabled;
          EEPROM.write(humidifierEnabledEEP, humidifierEnabled);
        break; // end case 3.

        case 4: // enable/disable fan:
          fanEnabled = !fanEnabled;
          EEPROM.write(fanEnabledEEP, fanEnabled);
        break; // end case 4.

        case 5: // enable/disable fan too:
          fanEnabled = !fanEnabled;
          EEPROM.write(fanEnabledEEP, fanEnabled);
        break; // end case 5.

        case 6: // enable/disable light.
          lightEnabled = !lightEnabled;
          EEPROM.write(lightEnabledEEP, lightEnabled);
        break; // end case 6.

        case 7: // enable/disable light too:
          lightEnabled = !lightEnabled;
          EEPROM.write(lightEnabledEEP, lightEnabled);
        break; // end case 7.

        default:
        break;
      } // end switch menu.
    break; // end select

    case 8: // up:
      switch(menu) {
        case 1: // up button in menu 1 currently unassigned.
        break; // end case 1.
        
        case 2: // increases temp setpoint: 
          setPointTemp ++;
          if (setPointTemp > 99) {
            setPointTemp = 99;
          } // end if.
          EEPROM.write(tempSetPointEEP, setPointTemp);
         break; // end case 2.

        case 3: // increase humidity setpoint:
          setPointHumidity ++;
          if (setPointHumidity >= 100) {
           setPointHumidity = 100;
          } //end if.
          EEPROM.write(humSetPointEEP, setPointHumidity);
        break; // end case 3.

        case 4: // increases fan interval:
          fanInterval ++; 
          if (fanInterval > 254) {
            fanInterval = 254;
          } // end if.
          EEPROM.write(fanIntervalEEP, fanInterval);
        break; // end case 4.

        case 5: // increases fan duration:
          fanDuration ++; 
          if (fanDuration > 30) {
            fanDuration = 30;
          } // end if.
          EEPROM.write(fanDurationEEP, fanDuration);
        break; // end case 5.

        case 6: // increases light on time by one hour:
          lightOnTime ++;
          if (lightOnTime > 16) {
            lightOnTime = 16;
          } // end if.
          EEPROM.write(lightOnTimeEEP, lightOnTime);
        break; // end case 6.

        case 7: // increases light off time by one hour:
          lightOffTime ++;
          if (lightOffTime > 24) {
            lightOffTime = 24;        
          } // end if.
          EEPROM.write(lightOffTimeEEP, lightOffTime);
        break; // end case 7.

        default:
        break;
      } // end switch(menu).
    break; // end up.

    case 4: // down:
      switch(menu) {
        case 1: // down button in menu 1 currently unassigned.
        break; 

        case 2: // decrease temp setpoint
          setPointTemp --;
          if (setPointTemp < 60) {
            setPointTemp = 60;
          } // end if.
          EEPROM.write(tempSetPointEEP, setPointTemp);
        break; // end case 2.

        case 3: // decrease hum setpoint
          setPointHumidity --;
          if (setPointHumidity <= 0) {
            setPointHumidity = 0;
          } // end if.
          EEPROM.write(humSetPointEEP, setPointHumidity);
        break; // end case 3.

        case 4: //decreases fan interval:
          fanInterval --;
          if (fanInterval < 1) {
            fanInterval = 1;
          } // end if.
          EEPROM.write(fanIntervalEEP, fanInterval);
        break; // end case 4.

        case 5: //decreases fan duration:
          fanDuration --;
          if (fanDuration <= 0) {
            fanDuration = 0;
          } // end if.
          EEPROM.write(fanDurationEEP, fanDuration);
        break; // end case 5.

        case 6: // decreases light on time by one hour:
          lightOnTime --;
          if (lightOnTime <= 0) {
            lightOnTime = 0;
          } // end if.
          EEPROM.write(lightOnTimeEEP, lightOnTime);
        break; // end case 6.

        case 7: // decreases light off time by one hour:
          lightOffTime --;
          if (lightOffTime < 8) {
            lightOffTime = 8;        
          } // end if.
          EEPROM.write(lightOffTimeEEP, lightOffTime);
        break; // end case 7.
      } // end switch(menu).
    break; // end down.

    case 2: // right:
      menu ++; //advances menu.
      if (menu > menuLimit) {
        menu = 1;
      } // end if.
      EEPROM.write(menuEEP, menu);
    break; // end right.

    case 16: //left
      menu --; //retreats menu.
      if (menu < 1) {
        menu = menuLimit;
      } // end if.
      EEPROM.write(menuEEP, menu);
    break; // end left.     
   } //end switch(buttons).    
  } //end if (buttons).
 
  digitalWrite(heatRelay,       heatOn);
  digitalWrite(fanRelay,        fanOn);
  digitalWrite(humidifierRelay, humidifierOn);

  // set LCD color depending on whats on.
  if (!heatOn && !fanOn && !humidifierOn) { //all off:
    lcd.setBacklight(WHITE); 
  } // end if.

  if (heatOn && !fanOn && !humidifierOn) { //heater on, rest off:
    lcd.setBacklight(RED);
  } // end if.

  if (!heatOn && !fanOn && humidifierOn) { //humidifier on, rest off:
    lcd.setBacklight(BLUE);
  } // end if.

  if (!heatOn && fanOn && !humidifierOn) { //fan on, rest off:
    lcd.setBacklight(YELLOW);
  } // end if.

  if (heatOn && !fanOn && humidifierOn) { // heat/humidifier on, fan off:
    lcd.setBacklight(VIOLET);
  } // end if.

  if (heatOn && fanOn && !humidifierOn) { // heat/fan on, humid off:
    lcd.setBacklight(TEAL);
  } // end if.

  if (!heatOn && fanOn && humidifierOn) { // fan/hum on, heat off:
    lcd.setBacklight(GREEN);
  } // end if.

  if (heatOn && fanOn && humidifierOn) { // all on, unlikely but hey what the hell:
    lcd.setBacklight(WHITE); //cause there are no more colors.
  } // end if.

  //light timer:
  if (hour >= lightOnTime && hour < lightOffTime && lightEnabled) {
    digitalWrite(lightRelay, HIGH);  
  } // end if.
  else {
    digitalWrite(lightRelay, LOW);
  } // end else.

  //get sensor readings:
  if (timeNow - sensorTimeLast > sensorReadInterval) {
    //oneWireTemp       = cToF(getOneWireTemp(sensor1)); // Get OneWire temp.
    oneWireTemp       = getOneWireTempF(sensor1); // Get OneWire temp.
        
    sht31temp   = float((sht31.readTemperature() * 1.8) + 32.0 + tempCalibration);
    
    humidity    = round(sht31.readHumidity());
    
    sensorTimeLast = timeNow;
    #ifdef debug
      Serial.print("OneWire Temp: ");
      Serial.print(oneWireTemp);
      Serial.println("F.");
      Serial.print("SHT31Temp   : ");
      Serial.print(sht31temp);
      Serial.println("F.");
      Serial.print("Humidity    : ");
      Serial.print(humidity);
      Serial.println("%.");
      Serial.println();
      
      Serial.print("Heater: ");
      if (heatEnabled) {
        Serial.println("Enabled");
      } // end if.
      else {
        Serial.println("Disabled");
      } // end else.
      
      Serial.print("Light: ");
      if (lightEnabled) {
        Serial.println("Enabled");
      } // end if.
      else {
        Serial.println("Disabled");
      } // end else.
      
      Serial.print("Fan: ");
      if (fanEnabled) {
        Serial.println("Enabled");
      } // end if.
      else {
        Serial.println("Disabled");
      } // end else.
            
      Serial.print("Humidifier: ");
      if (humidifierEnabled) {
        Serial.println("Enabled");
      } // end if.
      else {
        Serial.println("Disabled");
      } // end else
      Serial.print("SHT31 Heater: ");
      if (sht31.isHeaterEnabled()) {
        Serial.println("enabled.");
      } // end if.
      else {
        Serial.println("disabled.");
      } // end else.
      Serial.println();     
    #endif
  } // end if.

  // sht31 internal heater:
  if ((timeNow - sht31heaterTimer) > sht31heaterInterval) {
    sht31heater = true;
    sht31.heater(true);
    sht31heaterTimer  = timeNow;
    sht31heaterOnTime = timeNow;
  } // end if.

  if (((timeNow - sht31heaterOnTime) > sht31heaterDuration) && sht31heater) {
    sht31heater = false;
    sht31.heater(false);
  } // end if.

  // check and change heater status:
  if ((timeNow - heatTimeLast > heaterInterval) && heatEnabled) {
    if (oneWireTemp < setPointTemp) {
      heatOn = true;
      #ifdef debug
        Serial.println("Heater on.");
      #endif
    } // end if.
    else {
      heatOn = false;
      #ifdef debug
        Serial.println("Heater off.");
      #endif
    } // end else.
    heatTimeLast = timeNow;
  } // end if.

  // check and change fan status.
  if (( ((timeNow - fanTimer) / 60000 ) > (fanInterval)) && (fanDuration != 0) && fanEnabled) { // turns fan on, records time fan turned on.
    fanOn     = true;
    fanTimer  = timeNow;
    fanTimeOn = timeNow;
    #ifdef debug
      Serial.println("Fan on.");
    #endif
  } // end if.
  if ((((timeNow - fanTimeOn) / 1000) > fanDuration) && fanOn) { // turns fan off.
    fanOn = false;
    #ifdef debug
      Serial.println("Fan off.");
    #endif
  } // end if.

  // check and change humidifier status:
  if ((timeNow - humidifierTimeLast > humidifierInterval) && humidifierEnabled && (humidity !=0)) {
    if (humidity < setPointHumidity) {
      humidifierOn = true;
      #ifdef debug
        Serial.println("Humidifier on.");
      #endif
    } // end if.
    else {
      humidifierOn = false;
      #ifdef debug
        Serial.println("Humidifier off.");
      #endif
    } // end else.
    humidifierTimeLast = timeNow;
  } // end if.
 
  // top row of lcd screen:
  lcd.setCursor(0, 0);
  lcd.print    (F("Temp:"));
  lcd.setCursor(5, 0);
  lcd.print    (oneWireTemp);
  lcd.print    (F("F  "));
  lcd.setCursor(9, 0);
  lcd.print    (F("Hum:"));
  lcd.setCursor(13, 0);
  lcd.print    (humidity);
  lcd.print    (F("%  "));
  
  // bottom row:
  lcd.setCursor(0, 1);
  lcd.print    (F("M"));
  lcd.print    (menu);

  switch (menu) {
    case 1: // shows tSet, hSet:
      lcd.setCursor(3, 1);
      lcd.print    (F("Set T:"));
      lcd.setCursor(9, 1);
      lcd.print    (setPointTemp);
      lcd.print    (F("  "));
      lcd.setCursor(12, 1);
      lcd.print    (F("H:"));
      lcd.setCursor(14, 1);
      lcd.print    (setPointHumidity);
      lcd.print    (F("  "));
    break; // end case 1.

    case 2: // allows changing of t set:
      lcd.setCursor(3, 1);
      lcd.print    (F("Temp set: "));
      lcd.setCursor(13, 1);
      if (heatEnabled) {
        lcd.print    (setPointTemp);
        lcd.print    (F("F  "));
      } // end if.
      else {
        lcd.print(F("---   "));
      } // end if.
    break; // end case 2.

    case 3: // allows changing of h set:
      lcd.setCursor(3, 1);
      lcd.print    (F("Hum  set: "));
      lcd.setCursor(13, 1);
      if (humidifierEnabled) {
        lcd.print    (setPointHumidity);
        lcd.print    (F("%  "));
      } // end if.
      else {
        lcd.print(F("---     "));
      } // end else.
    break; // end case 3.

    case 4: // allows changing of fan interval:
      lcd.setCursor(3, 1);
      lcd.print    (F("FanEvery "));
      lcd.setCursor(12, 1);
      if (fanEnabled) {
        lcd.print    (fanInterval);
        lcd.print    (F("min  "));
      } // end if.
      else {
        lcd.print(F("---    "));
      } // end else.
      break; // end case 4.

    case 5: // allows changing of fan duration:
      lcd.setCursor(3, 1);
      lcd.print    (F("FanOnFor "));
      lcd.setCursor(12, 1);
      if (fanEnabled) {
        lcd.print    (fanDuration);
        lcd.print    (F("s  "));
      } // end if.
      else {
        lcd.print(F("---     "));
      } // end else.
    break; // end case 5.

    case 6: // allows changing of light on time:
      lcd.setCursor(3, 1);
      lcd.print(F("LiteOn:"));
      if (lightEnabled) {
        lcd.print(lightOnTime);
        lcd.print(F(":00   "));
      } // end if.
      else {
        lcd.print(F("---     "));
      } // end else.
    break; // end case 6.

    case 7: // allows changing of light off time:
      lcd.setCursor(3, 1);
      lcd.print(F("LiteOff:"));
      if (lightEnabled) {
        lcd.print(lightOffTime);
        lcd.print(F(":00   "));
      } // end if.
      else {
        lcd.print(F("---     "));
      } // end else.
    break; // end case 7.

    case 8: // shows sht31 temp:
      lcd.setCursor(3, 1);
      lcd.print    (F("FruitTemp:"));
      lcd.print    (round(sht31temp));
      lcd.print    (F("F "));
    break; // end case 8.

    case 9: // shows time:
      lcd.setCursor(3, 1);
      if (hour < 10) {
        lcd.print("0");
      } // end if.
      lcd.print(hour);
      lcd.print(":");
      if (minute < 10) {
        lcd.print("0");
      } // end if.
      lcd.print(minute);
      lcd.print(":");
      if (second < 10) {
        lcd.print("0");
      } // end if.
      lcd.print(second);
      lcd.print(F("       "));
    break; // end case 9.
  } // end switch(menu).
} // end loop().

int8_t cToF(float c) {                    // Convert float C to int F.
  return round((1.8 * c) + 32.0);
} // end cToF().

int8_t getOneWireTempF(byte *str) {       //get onewire temp in F.
  byte data[2];
  oneWire.reset ();
  oneWire.select(str);
  oneWire.write (0x44, 1);                // start conversion.
  oneWire.reset ();
  oneWire.select(str);
  oneWire.write (0xBE);                   // Read scratchpad.

  for (byte i = 0; i < 2; i ++) {
    data[i] = oneWire.read();             // Collect data.
  } // end for.

  int16_t raw = (data[1] << 8) | data[0]; // Convert raw data to C.
  float temp_c = (float)raw / 16.0;
  
  float temp_f = (temp_c * 1.8) + 32.0;
  return round(temp_f);
} // end getOneWireTempF().


int8_t getOneWireTemp(byte *str) {        // Get OneWire temp.
  byte data[2];                           // Array to store data retrieved from sensor.
  oneWire.reset ();
  oneWire.select(str);
  oneWire.write (0x44, 1);                // start conversion.
  oneWire.reset ();
  oneWire.select(str);
  oneWire.write (0xBE);                   // Read scratchpad.

  for (byte i = 0; i < 2; i ++) {
    data[i] = oneWire.read();             // Collect data.
  } // end for.

  int16_t raw = (data[1] << 8) | data[0]; // Convert raw data to C.
  return round((float)raw / 16.0);
} // end getOneWireTemp().
