void outOfrange() {
  oled.println(F("--"));
}

void SaveSettings() {
  // Save lightmeter setting into EEPROM.
  EEPROM.write(ndIndexAddr, ndIndex);
  EEPROM.write(ISOIndexAddr, ISOIndex);
  EEPROM.write(modeIndexAddr, modeIndex);
  EEPROM.write(apertureIndexAddr, apertureIndex);
  EEPROM.write(T_expIndexAddr, T_expIndex);
//  EEPROM.write(meteringModeAddr, meteringMode);
}

// Returns actual value of Vcc (x 100)
int getBandgap(void) {
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
  // For mega boards
  const long InternalReferenceVoltage = 1115L;  // Adjust this value to your boards specific internal BG voltage x1000
  // REFS1 REFS0          --> 0 1, AVcc internal ref. -Selects AVcc reference
  // MUX4 MUX3 MUX2 MUX1 MUX0  --> 11110 1.1V (VBG)         -Selects channel 30, bandgap voltage, to measure
  ADMUX = (0 << REFS1) | (1 << REFS0) | (0 << ADLAR) | (0 << MUX5) | (1 << MUX4) | (1 << MUX3) | (1 << MUX2) | (1 << MUX1) | (0 << MUX0);

#else
  // For 168/328 boards
  const long InternalReferenceVoltage = 1056L;  // Adjust this value to your boards specific internal BG voltage x1000
  // REFS1 REFS0          --> 0 1, AVcc internal ref. -Selects AVcc external reference
  // MUX3 MUX2 MUX1 MUX0  --> 1110 1.1V (VBG)         -Selects channel 14, bandgap voltage, to measure
  ADMUX = (0 << REFS1) | (1 << REFS0) | (0 << ADLAR) | (1 << MUX3) | (1 << MUX2) | (1 << MUX1) | (0 << MUX0);

#endif
  delay(50);  // Let mux settle a little to get a more stable A/D conversion
  // Start a conversion
  ADCSRA |= _BV( ADSC );
  // Wait for it to complete
  while ( ( (ADCSRA & (1 << ADSC)) != 0 ) );
  // Scale the value
  int results = (((InternalReferenceVoltage * 1024L) / ADC) + 5L) / 10L; // calculates for straight line value

  return results;
} 

void footer() {
  oled.setCursor(0, 55);
  oled.print(F("press M"));
}

/*
  Get light value
*/
float getLux() {
  uint16_t lux = lightMeter.readLightLevel();

  if (lux >= 65534) {
    // light sensor is overloaded.
    Overflow = 1;
    lux = 65535;
  } else {
    Overflow = 0;
  }

  return lux * DomeMultiplier;             // DomeMultiplier = 2.17 (calibration)*/
}

float log2(float x) {
  return log(x) / log(2);
}

float lux2ev(float lux) {
  return log2(lux / 2.5);
}

// return aperture value (1.4, 1.8, 2.0) by index in sequence (0, 1, 2, 3, ...).
float getApertureByIndex(uint8_t indx) {
  float roundIndx = 10.0;

  if (indx > 39) {
    roundIndx = 1;
  }

  float f = round(pow(2, indx / 3.0 * 0.5) * roundIndx) / roundIndx;

  // the formula returns exact value, but photographers uses more memorable values.
  // convert it.
/**
  if (f >= 1.1 && f < 1.2) {
    f = 1.1;
  } else if (f >= 1.2 && f < 1.4) {
    f = 1.2;
  } else if (f > 3.2 && f < 4) {
    f = 3.5;
  } else if (f > 5 && f < 6.3) {
    f = 5.6;
  } else if (f > 10 && f < 11) {
    f = 10;
  } else if (f >= 11 && f < 12) {
    f = 11;
  } else if (f >= 12 && f < 14) {
    f = 13;
  } else if (f >= 14 && f < 16) {
    f = 14;
  } else if (f >= 20 && f < 22) {
    f = 20;
  } else if (f >= 22 && f < 25) {
    f = 22;
  } else if (f >= 24 && f < 28) {
    f = 25;
  } else if (f >= 28 && f < 40) {
    f = 36;
  } else if (f >= 40 && f < 45) {
    f = 40;
  } else if (f >= 45 && f < 50) {
    f = 45;
  } else if (f >= 50 && f < 57) {
    f = 51;
  } else if (f >= 71 && f < 80) {
    f = 72;
  } else if (f >= 80 && f < 90) {
    f = 80;
  } else if (f >= 90 && f < 101) {
    f = 90;
  } */

  return f;
}

// Return ISO value (100, 200, 400, ...) by index in sequence (0, 1, 2, 3, ...).

float getISOByIndex(uint8_t indx) {
  if (indx < 0 || indx > MaxISOIndex) {
    indx = 0;
  }

  float isoValues[] = {.8, 1, 1.25, 1.6, 2, 2.5, 3.2, 4, 5, 6.4};
  float iso = isoValues[indx % 10] * pow(10, floor(indx / 10));

  return iso;

}

float getMinDistance(float x, float v1, float v2) {
  if (x - v1 > v2 - x) {
    return v2;
  }

  return v1;
}

float getTimeByIndex(uint8_t indx) {
  if (indx < 0 || indx >= MaxTimeIndex) {
    indx = 0;
  }

  float shutterspeedvalues[] = {100, 80, 64, 50, 40, 32, 25, 20, 15, 12.5};
  float factor = floor(indx / 10) - 2; // get multiplying factor
  factor = -factor; // invert
  float t = shutterspeedvalues[indx % 10] * pow(10, factor);
  t = 1 / t;
  return t;
}

// Convert calculated time (in seconds) to photograpy style shutter speed. 


double fixTime(double t) {
  double divider = 1;

  float maxTime = getTimeByIndex(MaxTimeIndex);

  if (t < maxTime) {
    return maxTime;
  }

/*
  t = 1 / t;

  if (t > 99999) {
    divider = 10000;
  } else if (t > 9999) {
    divider = 1000;
  } else if (t > 999) {
    divider = 100;
  } else if (t > 99) {
    divider = 10;
  }

  t = t / divider;

  if (t >= 10 && t <= 12.5) {
    t = getMinDistance(t, 10, 12.5);
  } else if (t >= 12.5 && t <= 16) {
    t = getMinDistance(t, 12.5, 16);
  } else if (t >= 16 && t <= 20) {
    t = getMinDistance(t, 16, 20);
  } else if (t >= 20 && t <= 25) {
    t = getMinDistance(t, 20, 25);
  } else if (t >= 25 && t <= 32) {
    t = getMinDistance(t, 25, 32);
  } else if (t >= 32 && t <= 40) {
    t = getMinDistance(t, 32, 40);
  } else if (t >= 40 && t <= 50) {
    t = getMinDistance(t, 40, 50);
  } else if (t >= 50 && t <= 64) {
    t = getMinDistance(t, 50, 64);
  } else if (t >= 64 && t <= 80) {
    t = getMinDistance(t, 64, 80);
  } else if (t >= 80 && t <= 100) {
    t = getMinDistance(t, 80, 100);
  }

  t = t * divider;

  if (t == 32) {
    t = 30;
  }

  if (t == 16) {
    t = 15;
  }

  t = 1 / t;
*/
  return t;
}

// Convert calculated aperture value to photograpy style aperture value. 
float fixAperture(float a) {
  for (int i = 0; i < MaxApertureIndex; i++) {
    float a1 = getApertureByIndex(i);
    float a2 = getApertureByIndex(i + 1);

    if (a1 < a && a2 >= a) {
      return getMinDistance(a, a1, a2);
    }
  }

  return 0;
}

/*
  Return ND from ndIndex
  int ND[] = {0, 3, 6, 9, 12, 15, 18, 21, 24, 27, 30, 33, 36, 39, 42, 45, 48}; // eg.: 1) 0.3 ND = -1 stop = 2^2 = 4; 2) 0.9 ND = -3 stop = 2^3 = 16;
*/
uint8_t getND(uint8_t ndIndex) {
  if (ndIndex == 0) {
    return 0;
  }

  return 3 + (ndIndex - 1) * 3;
}

// Calculate new exposure value and display it.

void refresh() {
  ISOMenu = false;
  mainScreen = true;
  NDMenu = false;

  float EV = lux2ev(lux);

  float T = getTimeByIndex(T_expIndex);
  float A = getApertureByIndex(apertureIndex);
  float iso = getISOByIndex(ISOIndex);
  uint8_t ndStop = getND(ndIndex);

  // if ND filter is configured then make corrections.
  // As ISO is a main operand in all EV calculations we can adjust ISO by ND filter factor.
  // if ND4 (ND 0.6) filter is configured then we need to adjust ISO to -2 full stops. Ex. 800 to 200
  if (ndIndex > 0) {
    ISOND = iso / (pow(2, ndIndex));
  } else {
    ISOND = iso;
  }

  if (lux > 0) {
    if (modeIndex == 0) {
      // Aperture priority. Calculating time.
      T = fixTime(100 * pow(A, 2) / ISOND / pow(2, EV)); //T = exposure time, in seconds

      // Calculating shutter speed index for correct menu navigation.
      for (int i = 0; i <= MaxTimeIndex; i++) {
        if (T == getTimeByIndex(i)) {
          T_expIndex = i;
          break;
        }
      }
    } else if (modeIndex == 1) {
      // Shutter speed priority. Calculating aperture.
      A = fixAperture(sqrt(pow(2, EV) * ISOND * T / 100));

      // Calculating aperture index for correct menu navigation.
      if (A > 0) {
        for (int i = 0; i <= MaxApertureIndex; i++) {
          if (A == getApertureByIndex(i)) {
            apertureIndex = i;
            break;
          }
        }
      }
    }
  } else {
    if (modeIndex == 0) {
      T = 0;
    } else {
      A = 0;
    }
  }

  uint8_t Tdisplay = 0; // Flag for shutter speed display style (fractional, seconds, minutes)
  double  Tfr = 0;
  float   Tmin = 0;

  if (T >= 60) {
    Tdisplay = 0;  // Exposure is in minutes
    Tmin = T / 60;

  } else if (T < 60 && T >= 0.5) {
    Tdisplay = 2;  // Exposure in in seconds

  } else if (T < 0.5) {
    Tdisplay = 1;  // Exposure is in fractional form
    Tfr = round(1 / T);
  }

// Begin screen print:

  uint8_t linePos[] = {3, 5};
  oled.clear();
  oled.set1X();
  oled.setCursor(13, 1);
  oled.print(F("ISO:"));

  if (iso > 13) {
    oled.print(iso, 0);
  } else {
    oled.print(iso, 1);
  } 

  oled.setCursor(0, 2);
  oled.print(F("---------------------"));

// Display f/stop

  oled.setCursor(10, linePos[0]);

  oled.set2X();
  oled.print(F("f/"));
  if (A > 0) {
    if (A >= 100) {
      oled.print(A, 0);
    } else {
      oled.print(A, 1);
    }
  } else {
    outOfrange();
  }

//  display.setTextSize(1);

  // battery indicator
//  display.drawRect(122, 1, 6, 8, WHITE);
//  display.drawLine(124, 0, 125, 0, WHITE);


  // battery indicator for 2 elements of 1.5v each.
  if (battVolts > 270) {
    // full
    // display.fillRect(123, 1, 4, 7, WHITE);
    oled.setCursor(122, 0);
    oled.print("F");
  } else if (battVolts > 240) {
    // medium
    // display.fillRect(123, 4, 4, 5, WHITE);
        oled.setCursor(122, 0);
    oled.print("M");

  } else if (battVolts > 210) {
    // minimum
    // display.fillRect(123, 6, 4, 3, WHITE);
    oled.setCursor(122, 0);
    oled.print("L");

  } else {
    oled.setCursor(122, 0);
    oled.print("E");

  }

/*
  // Metering mode icon
  oled.setCursor(0, 1);
  if (meteringMode == 0) {
    // Ambient light
    oled.print(F("A"));
  } else if (meteringMode == 1) {
    // Flash light
    oled.print(F("F"));
  }
  // End of metering mode icon

*/
  oled.setCursor(67, 1);
  oled.set1X();
  oled.print(F("lux:"));
  oled.print(lux, 0);

//  display.drawLine(95, linePos[0] - 1, 95, linePos[0] + 17, WHITE); // LINE DIVISOR
//  display.setTextSize(1);
  oled.setCursor(100, linePos[0]);
  oled.print(F("| EV: "));
  oled.setCursor(100, linePos[0] + 1);
  if (lux > 0) {
    oled.print(F("| "));	
    oled.println(EV, 0);
  } else {
    oled.println(0, 0);
  }
/*
// ND filter indicator
  if (ndIndex > 0) {
    // display.drawLine(0, 55, 128, 55, WHITE); // LINE DIVISOR
    // display.setTextSize(1);
    oled.setCursor(0, 57);
    oled.print(F("ND"));
    // display.setCursor(100, linePos[0] + 10);
    oled.print(pow(2, ndIndex), 0);
    oled.print(F("="));
    oled.println(ndStop / 10.0, 1);
  }
*/
//  display.setTextSize(2);
  oled.set2X();
  oled.setCursor(10, linePos[1]);
  oled.print(F("T:"));

  if (Tdisplay == 0) {
    oled.print(Tmin, 1);
    oled.print(F("m"));
  } else if (Tdisplay == 1) {
    if (T > 0) {
      oled.print(F("1/"));
      oled.print(Tfr, 0);
    } else {
      outOfrange();
    }
  } else if (Tdisplay == 2) {
    oled.print(T, 1);
    oled.print(F("s"));
  } else if (Tdisplay == 3) {
    outOfrange();
  }

  // priority marker (shutter or aperture priority indicator)
//  display.setTextSize(1);
  oled.set1X();
  oled.setCursor(0, linePos[modeIndex] + 1);
  oled.print(F("*"));

//  oled.display();
}

void showISOMenu() {
  ISOMenu = true;
  NDMenu = false;
  mainScreen = false;

  oled.clear();
  oled.set2X();
  oled.setCursor(50, 0);
  oled.println(F("ISO:"));

  float iso = getISOByIndex(ISOIndex);
  oled.setCursor(51, 3);
  if (iso > 13) {
    oled.print(iso, 0);
  } else {
    oled.print(iso, 1);
  }
  delay(200);
}
/* 
void showNDMenu() {
  ISOMenu = false;
  mainScreen = false;
  NDMenu = true;

  oled.clear();
//  oled.setTextSize(2);
  oled.setCursor(10, 1);
  oled.println(F("ND Filter"));
//  display.setTextSize(3);

  if (ndIndex > 9) {
    oled.setCursor(10, 40);
  } else if (ndIndex > 6) {
    oled.setCursor(20, 40);
  } else if (ndIndex > 3) {
    oled.setCursor(30, 40);
  } else {
    oled.setCursor(40, 40);
  }

  if (ndIndex > 0) {
    oled.print(F("ND"));
    oled.print(pow(2, ndIndex), 0);
  } else {
//    oled.setTextSize(2);
    oled.setCursor(10, 4);
    oled.print(F("No filter"));
  }

//  display.display();
  delay(200);
}
*/
// Navigation menu
void menu() {
    if (MenuButtonState == 0) {
    if (mainScreen) {
      showISOMenu();
 //   }  else if (ISOMenu) {
//      showNDMenu();
    } else {
      refresh();
      delay(200);
    }
  }
/*
  if (NDMenu) {
    if (PlusButtonState == 0) {
      ndIndex++;

      if (ndIndex > MaxNDIndex) {
        ndIndex = 0;
      }
    } else if (MinusButtonState == 0) {
      if (ndIndex <= 0) {
        ndIndex = MaxNDIndex;
      } else {
        ndIndex--;
      }
    }

    if (PlusButtonState == 0 || MinusButtonState == 0) {
      showNDMenu();
    } 
  } */


  if (ISOMenu) {
    // ISO change mode
    if (PlusButtonState == 0) {
      // increase ISO
      ISOIndex++;

      if (ISOIndex > MaxISOIndex) {
        ISOIndex = 0;
      }
    } else if (MinusButtonState == 0) {
      if (ISOIndex > 0) {
        ISOIndex--;
      } else {
        ISOIndex = MaxISOIndex;
      }
    }

    if (PlusButtonState == 0 || MinusButtonState == 0) {
      showISOMenu();
    }
  }

  if (ModeButtonState == 0) {
    // switching between Aperture priority and Shutter Speed priority.
    if (mainScreen) {
      modeIndex++;

      if (modeIndex > 1) {
        modeIndex = 0;
      }
    }

    refresh();
    delay(200);
  }
/*
  if (mainScreen && MeteringModeButtonState == 0) {
    // Switch between Ambient light and Flash light metering
    if (meteringMode == 0) {
      meteringMode = 1;
    } else {
      meteringMode = 0;
    }

    refresh();
    delay(200);
  }
*/
  if (mainScreen && (PlusButtonState == 0 || MinusButtonState == 0)) {
    if (modeIndex == 0) {
      // Aperture priority mode
      if (PlusButtonState == 0) {
        // Increase aperture.
        apertureIndex++;

        if (apertureIndex > MaxApertureIndex) {
          apertureIndex = 0;
        }
      } else if (MinusButtonState == 0) {
        // Decrease aperture
        if (apertureIndex > 0) {
          apertureIndex--;
        } else {
          apertureIndex = MaxApertureIndex;
        }
      }
    } else if (modeIndex == 1) {
      // Time priority mode
      if (PlusButtonState == 0) {
        // increase time
        T_expIndex++;

        if (T_expIndex > MaxTimeIndex) {
          T_expIndex = 0;
        }
      } else if (MinusButtonState == 0) {
        // decrease time
        if (T_expIndex > 0) {
          T_expIndex--;
        } else {
          T_expIndex = MaxTimeIndex;
        }
      }
    }

    delay(200);

    refresh();
  }
}

/*
  Read buttons state
*/
void readButtons() {
  PlusButtonState = digitalRead(PlusButtonPin);
  MinusButtonState = digitalRead(MinusButtonPin);
  MeteringButtonState = digitalRead(MeteringButtonPin);
  ModeButtonState = digitalRead(ModeButtonPin);
  MenuButtonState = digitalRead(MenuButtonPin);
//  MeteringModeButtonState = digitalRead(MeteringModeButtonPin);
}
