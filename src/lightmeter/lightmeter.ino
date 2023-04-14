// #include <SPI.h>
#include <Wire.h>
#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"
#include <BH1750.h>
#include <EEPROM.h>
// #include <avr/sleep.h> 

// 0X3C+SA0 - 0x3C or 0x3D
#define I2C_ADDRESS 0x3C

// Define proper RST_PIN if required.
#define RST_PIN -1

SSD1306AsciiWire oled;

BH1750 lightMeter;

// #define DomeMultiplier          2.17                 // Multiplier when using a white translucid Dome covering the lightmeter
#define DomeMultiplier          1                                    
#define MeteringButtonPin       2                       // Metering button pin
#define PlusButtonPin           3                       // Plus button pin
#define MinusButtonPin          4                       // Minus button pin
#define ModeButtonPin           5                       // Mode button pin
#define MenuButtonPin           6                       // ISO button pin
// #define MeteringModeButtonPin         7                       // Metering Mode (Ambient / Flash)
//#define PowerButtonPin          2

#define MaxISOIndex             40
#define MaxApertureIndex        39
#define MaxTimeIndex            200
#define MaxNDIndex              13
// #define MaxFlashMeteringTime    5000                    // ms

float   lux;
boolean Overflow = 0;                                   // Sensor got Saturated and Display "Overflow"
float   ISOND;
boolean ISOmode = 0;
boolean NDmode = 0;

boolean PlusButtonState;                // "+" button state
boolean MinusButtonState;               // "-" button state
boolean MeteringButtonState;            // Metering button state
boolean ModeButtonState;                // Mode button state
boolean MenuButtonState;                // ISO button state
// boolean NDModeButtonState;        // Metering mode button state (Ambient / Flash)

boolean ISOMenu = false;
boolean NDMenu = false;
boolean mainScreen = false;

// EEPROM for memory recording
#define ISOIndexAddr        1
#define apertureIndexAddr   2
#define modeIndexAddr       3
#define T_expIndexAddr      4
// #define meteringModeAddr    5
#define ndIndexAddr         6

#define defaultApertureIndex 12
#define defaultISOIndex      21
#define defaultModeIndex     0
#define defaultT_expIndex    19

uint8_t ISOIndex =          EEPROM.read(ISOIndexAddr);
uint8_t apertureIndex =     EEPROM.read(apertureIndexAddr);
uint8_t T_expIndex =        EEPROM.read(T_expIndexAddr);
uint8_t modeIndex =         EEPROM.read(modeIndexAddr);
// uint8_t meteringMode =      EEPROM.read(meteringModeAddr);
uint8_t ndIndex =           EEPROM.read(ndIndexAddr);


float battVolts;
#define batteryInterval 10000
double lastBatteryTime = 0;

float spvalues[] = {100, 80, 60, 50, 40, 30, 25, 20, 15, 12.5, 10};
float apvalues[] = {1, 1.1, 1.3, 1.4, 1.6, 1.8, 2, 2.2, 2.5, 2.8, 3.2, 3.5, 4, 4.5, 5, 5.6, 6.3, 7.1, 8, 9, 10, 11, 12.5, 14, 16, 18, 20, 22, 25, 28, 32, 36, 40, 45, 50, 57, 64, 72, 80, 90};
float isoValues[] = {.8, 1, 1.25, 1.6, 2, 2.5, 3.2, 4, 5, 6.4};
float batteryvalues[] = {2.5, 2.7, 3.0, 3.5};
char batterystatus[4][1] = {"E", "L", "M", "F"};

#include "lightmeterascii.h"

void setup() {  
  pinMode(PlusButtonPin, INPUT_PULLUP);
  pinMode(MinusButtonPin, INPUT_PULLUP);
  pinMode(MeteringButtonPin, INPUT_PULLUP);
  pinMode(ModeButtonPin, INPUT_PULLUP);
  pinMode(MenuButtonPin, INPUT_PULLUP);
 //  pinMode(MeteringModeButtonPin, INPUT_PULLUP);

// Serial.begin(115200);

  battVolts = getBandgap();  //Determins what actual Vcc is, (X 100), based on known bandgap voltage

  Wire.begin();
  lightMeter.begin(BH1750::ONE_TIME_HIGH_RES_MODE_2);
  //lightMeter.begin(BH1750::ONE_TIME_LOW_RES_MODE); // for low resolution but 16ms light measurement time.

  oled.begin(&Adafruit128x64, I2C_ADDRESS);
  oled.setFont(Adafruit5x7);
  oled.clear();


  // IF NO MEMORY WAS RECORDED BEFORE, START WITH THIS VALUES otherwise it will read "255"
  if (apertureIndex > MaxApertureIndex) {
    apertureIndex = defaultApertureIndex;
  }

  if (ISOIndex > MaxISOIndex) {
    ISOIndex = defaultISOIndex;
  }

  if (T_expIndex > MaxTimeIndex) {
    T_expIndex = defaultT_expIndex;
  }

  if (modeIndex < 0 || modeIndex > 1) {
    // Aperture priority. Calculating shutter speed.
    modeIndex = 0;
  }

  /* if (meteringMode > 1) {
    meteringMode = 0;
  }
*/
  if (ndIndex > MaxNDIndex) {
    ndIndex = 0;
  }

  lux = getLux();
  refresh();
}

void loop() {  
//  if (millis() >= lastBatteryTime + batteryInterval) {
//    lastBatteryTime = millis();
//    battVolts = getBandgap();
//  }
  
  readButtons();

  menu();

  if (MeteringButtonState == 0) {
    // Save setting if Metering button pressed.
    SaveSettings();

    lux = 0;
    // refresh();
    oled.clear(); 
    // if (meteringMode == 0) {
      // Ambient light meter mode.
      lightMeter.configure(BH1750::ONE_TIME_HIGH_RES_MODE_2);
      delay(500); // give 1/2 sec for things to settle
      lux = getLux();

      if (Overflow == 1) {
        delay(10);
        getLux();
      }

      refresh();
      delay(200);
    /*
    } else if (meteringMode == 1) {
      // Flash light metering
      lightMeter.configure(BH1750::CONTINUOUS_LOW_RES_MODE);

      unsigned long startTime = millis();
      uint16_t currentLux = 0;
      lux = 0;

      while (true) {
        // check max flash metering time
        if (startTime + MaxFlashMeteringTime < millis()) {
          break;
        }

        currentLux = getLux();
        delay(16);
        
        if (currentLux > lux) {
          lux = currentLux;
        }
      }

      refresh();
    } */
  }
}
