#include <Arduino.h>
#include <RTClib.h>
#include <Wire.h>
#include "LiquidCrystal_I2C.h"
#include <EEPROM.h>
/*
  AUTOMATED EBB AND FLOW HYDROPONIC SYSTEM
  CONFIGURABLE WITH ARDUINO MEGA, NANO & UNO
  WRITTEN BY CEDRIC MCENROE
*/

//---------Initialize I2C Devices----------
RTC_DS1307 rtc;
LiquidCrystal_I2C lcd(0x27, 20, 4);

//----------Set Global Variables-----------
// Select your targeted board and remove / add comments where necessary
// Arduino Mega
int Aval, Bval, Cval, Dval, Eval, Fval, Gval, Hval, Ival, Jval, Kval, Lval, Mval, Nval = 0;
int numPins = 12;           //Default Number of Sensors, can be adjusted in menu (automatically?)
int maxPins = 14;           //Maximum Number of Configurable Sensors
bool tempC  = true;         //Enable Temperature Sensor
int boardnum= 0;            //Set Board Number
int EEPNumpins = 0;         //numPins EEPROM memory address
int* pinValues;   // Pointer to store pin values
int wateringTime = 300; // Watering time in seconds


/*
MEGA PINOUT:
A0 - A13 -> Moisture Sensor (Yellow Wire)
A14      -> Temperature SDA
A15      -> Temperature SCL
SDA1     -> RTC SDA
SCL1     -> RTC SCL
SDA      -> LCD SDA
SCL      -> LCD SCL
D2       -> UP Button
D3       -> DOWN Button
D5       -> SELECT Button
D10      -> Light Relay Signal
D11      -> Pump Relay Signal
*/

/*
// Arduino Uno / Nano
int Aval, Bval, Cval, Dval; //Up to 4 sensors (Arduino Uno, Arduino Nano)
int numPins = 4;            //Default Number of Sensors, can be adjusted in menu (automatically?)
int maxPins = 4;            //Maximum Number of Configurable Sensors
bool tempC = false;         //Disable Temperature Sensor
int boardnum= 1;
int EEPNumpins = 0;         //numPins EEPROM memory address
int* pinValues;   // Pointer to store pin values
int wateringTime = 300; // Watering time in seconds
/*
UNO / NANO PINOUT:
A0 - A3  -> Moisture Sensor (Yellow Wire)
SDA / A6 -> RTC SDA
SCL / A7 -> RTC SCL
A4       -> LCD SDA
A5       -> LCD SCL
D2       -> UP Button
D3       -> DOWN Button
D5       -> SELECT Button
D10      -> Light Relay Signal
D11      -> Pump Relay Signal
*/

float average = 0.0;        //Sensor Average, to be printed on LCD during stats

// Relays
int LightRelay = 10;        // Digital pin connected to the Light Relay
int PumpRelay = 11;         // Digital pin connected to the Pump Relay

// Navigation Buttons
const int upPin = 2;        // Digital pin connected to Up Button
const int downPin = 3;      // Digital pin connected to Down Button
const int selectPin = 5;    // Digital pin connected to Select Button
const int debounceDelay = 80;  // Adjust the delay value as needed

// Light Timing
int DAY = 8;
int NIGHT = 20;

//----------Menu Setup----------
enum MenuState{
  MAIN_MENU,
  MANUAL_CONTROL,
  SETUP,
  STATS,
  SENSOR_ADJUST,
  LIGHT_ADJUST,
  TOGGLE_LIGHTS,
  TOGGLE_PUMP,
  WADJUST
};
MenuState currentMenu = STATS; //Default Menu State
int selectedMenuIndex = 1;

void setup() {
  Serial.begin(9600);
  EEPROM.begin();
  Wire.begin();
  rtc.begin();
  pinValues = new int[maxPins]; // Dynamically allocate memory for pin values
  if (!rtc.isrunning()) {
    Serial.println("RTC is NOT running!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  if (tempC) {
    // Initialize the Temperature and Humidity Sensor HERE
  }

  pinMode(LightRelay, OUTPUT);
  pinMode(PumpRelay, OUTPUT);

  lcd.begin(20, 4);
  pinMode(selectPin, INPUT_PULLUP);
  pinMode(upPin, INPUT_PULLUP);
  pinMode(downPin, INPUT_PULLUP);
  lcd.backlight();
}

void loop() {
  //Menu Switch
  numPins = EEPROM.read(0);
  switch (currentMenu) {
    case MAIN_MENU:
      MainMenu();
      break;
    case MANUAL_CONTROL:
      ManualMenu();
      break;
    case SETUP:
      SetupMenu();
      break;
    case STATS:
      DisplayStats();
      break;
    case SENSOR_ADJUST:
      SensorAdjust();
      break;
    case LIGHT_ADJUST:
      LightAdjust();
      break;
    case TOGGLE_LIGHTS:
      ToggleLights();
      break;
    case TOGGLE_PUMP:
      TogglePump();
      break;
    case WADJUST:
      wateringTimeAdjust();
      break;
  }
}

void DisplayStats() {
  delay(500);
  lcd.clear();
  while (currentMenu == STATS) {
    if (digitalRead(selectPin) == LOW) {
      delay(debounceDelay);
      if (digitalRead(selectPin) == LOW) {
        Serial.println("Select Pressed");
        currentMenu = MAIN_MENU;
        break;
      }
    }

    DateTime now = rtc.now();
    int hour = now.hour();
    int minute = now.minute();
    int second = now.second();

    if (now.hour() >= DAY && now.hour() < NIGHT) {
      digitalWrite(LightRelay, HIGH);
      Serial.println("Lights turned on.");
    } else {
      digitalWrite(LightRelay, LOW);
      Serial.println("Lights turned off.");
    }
    getAverage();

    Serial.print("Average: ");
    Serial.println(average);

    lcd.setCursor(0, 0);
    lcd.print("SOIL: ");
    lcd.print(average);

    if (average <= 400) {
      lcd.print(" WET");
    } else if (average > 400) {
      lcd.print(" DRY");
    }

    lcd.setCursor(0, 1);
    lcd.print("TIME: ");
    lcd.print(now.hour(), DEC);
    lcd.print(":");

    if (now.minute() < 10) {
      lcd.print(0);
    }
    lcd.print(now.minute(), DEC);

    if (tempC) {
      // Display the Temperature and Humidity Sensor HERE
    }

    if (average >= 400) {
      // Call the TogglePump() function or set currentMenu to TOGGLE_PUMP
      currentMenu = TOGGLE_PUMP;
      break;
    }

    delay(250);
  }
}

void MainMenu() {
  delay(500);
  lcd.clear();
  while (currentMenu == MAIN_MENU) {
    lcd.setCursor(0, 0);
    lcd.print("Main Menu");

    if (selectedMenuIndex == 1) {
      lcd.setCursor(0, 1);
      lcd.print("> Manual Control");
      if (digitalRead(selectPin) == LOW) {
        delay(debounceDelay);
        if (digitalRead(selectPin) == LOW) {
          Serial.println("Select Pressed");
          currentMenu = MANUAL_CONTROL;
          break;
        }
      }
    } else {
      lcd.setCursor(0, 1);
      lcd.print("  Manual Control");
    }

    if (selectedMenuIndex == 2) {
      lcd.setCursor(0, 2);
      lcd.print("> Setup");
      if (digitalRead(selectPin) == LOW) {
        delay(debounceDelay);
        if (digitalRead(selectPin) == LOW) {
          Serial.println("Select Pressed");
          currentMenu = SETUP;
          break;
        }
      }
    } else {
      lcd.setCursor(0, 2);
      lcd.print("  Setup");
    }

    if (selectedMenuIndex == 3) {
      lcd.setCursor(0, 3);
      lcd.print("> Return");
      if (digitalRead(selectPin) == LOW) {
        delay(debounceDelay);
        if (digitalRead(selectPin) == LOW) {
          Serial.println("Select Pressed");
          currentMenu = STATS;
          break;
        }
      }
    } else {
      lcd.setCursor(0, 3);
      lcd.print("  Return");
    }
    menuNavigation();
  }
}

void SetupMenu() {
  delay(500);
  lcd.clear();
  while (currentMenu == SETUP) {
    lcd.setCursor(0, 0);
    lcd.print("Setup Menu");

    if (selectedMenuIndex == 1) {
      lcd.setCursor(0, 1);
      lcd.print("> Configure Sensors");
      if (digitalRead(selectPin) == LOW) {
        delay(debounceDelay);
        if (digitalRead(selectPin) == LOW) {
          Serial.println("Select Pressed");
          currentMenu = SENSOR_ADJUST;
          break;
        }
      }
    } else {
      lcd.setCursor(0, 1);
      lcd.print("  Configure Sensors");
    }

    if (selectedMenuIndex == 2) {
      lcd.setCursor(0, 2);
      lcd.print("> Adjust Lights");
      if (digitalRead(selectPin) == LOW) {
        delay(debounceDelay);
        if (digitalRead(selectPin) == LOW) {
          Serial.println("Select Pressed");
          currentMenu = LIGHT_ADJUST;
          break;
        }
      }
    } else {
      lcd.setCursor(0, 2);
      lcd.print("  Adjust Lights");
    }

    if (selectedMenuIndex == 3) {
      lcd.setCursor(0, 3);
      lcd.print("> Return");
      if (digitalRead(selectPin) == LOW) {
        delay(debounceDelay);
        if (digitalRead(selectPin) == LOW) {
          Serial.println("Select Pressed");
          currentMenu = MAIN_MENU;
          break;
        }
      }
    } else {
      lcd.setCursor(0, 3);
      lcd.print("  Return");
    }
    menuNavigation();
  }
}

void ManualMenu() {
  delay(500);
  lcd.clear();
  while (currentMenu == MANUAL_CONTROL) {
    lcd.setCursor(0, 0);
    lcd.print("Manual Control");
  
    if (selectedMenuIndex == 1) {
      lcd.setCursor(0, 1);
      lcd.print("> Toggle Lights");
      if (digitalRead(selectPin) == LOW) {
        delay(debounceDelay);
        if (digitalRead(selectPin) == LOW) {
          Serial.println("Select Pressed");
          currentMenu = TOGGLE_LIGHTS;
          break;
        }
      }
    } else {
      lcd.setCursor(0, 1);
      lcd.print("  Toggle Lights");
    }
  
    if (selectedMenuIndex == 2) {
      lcd.setCursor(0, 2);
      lcd.print("> Toggle Pump");
      if (digitalRead(selectPin) == LOW) {
        delay(debounceDelay);
        if (digitalRead(selectPin) == LOW) {
          Serial.println("Select Pressed");
          currentMenu = TOGGLE_PUMP;
          break;
        }
      }
    } else {
      lcd.setCursor(0, 2);
      lcd.print("  Toggle Pump");
    }
  
    if (selectedMenuIndex == 3) {
      lcd.setCursor(0, 3);
      lcd.print("> Return");
      if (digitalRead(selectPin) == LOW) {
        delay(debounceDelay);
        if (digitalRead(selectPin) == LOW) {
          Serial.println("Select Pressed");
          currentMenu = MAIN_MENU;
          break;
        }
      }
    } else {
      lcd.setCursor(0, 3);
      lcd.print("  Return");
    }
    menuNavigation();
  }
}

void LightAdjust() {
  //ADD TO EEPROM
  delay(500);
  lcd.clear();
  while (currentMenu == LIGHT_ADJUST) {
    lcd.setCursor(0, 0);
    lcd.print("Adjust Lighting Time");

    if (selectedMenuIndex == 1) {
      lcd.setCursor(0, 1);
      lcd.print("> Day: ");
      if(DAY<10){
        lcd.print(0);
      }
      lcd.print(DAY);
      if (digitalRead(selectPin) == LOW) {
        delay(debounceDelay);
        if (digitalRead(selectPin) == LOW) {
          Serial.println("Select Pressed");
          bool runit = true;
          while (runit) {
            lcd.setCursor(0, 1);
            lcd.print("* Day: ");
            if(DAY<10){
              lcd.print("0");
            }
            lcd.print(DAY);
            if (digitalRead(upPin) == LOW) {
              delay(debounceDelay);
              if (digitalRead(upPin) == LOW && DAY < 24) {
                Serial.println("Up Pressed");
                DAY++;
              } else if (digitalRead(upPin) == LOW && DAY >= 24) {
                Serial.println("Up Pressed");
                DAY = 0;
              }
            }

            if (digitalRead(downPin) == LOW) {
              delay(debounceDelay);
              if (digitalRead(downPin) == LOW && DAY > 0) {
                Serial.println("Down Pressed");
                DAY--;
              } else if (digitalRead(downPin) == LOW && DAY <= 0) {
                Serial.println("Down Pressed");
                DAY = 24;
              }
            }

            if (digitalRead(selectPin) == LOW) {
              Serial.println("Select Pressed");
              lcd.setCursor(0, 1);
              lcd.print("> Day: ");
              if(DAY < 10){
                lcd.print("0");
              }
              lcd.print(DAY);
              runit = false;
            }
          }
        }
      }
    } else {
      lcd.setCursor(0, 1);
      lcd.print("  Day: ");
      if(DAY < 10){
        lcd.print("0");
      }
      lcd.print(DAY);
    }

    if (selectedMenuIndex == 2) {
      lcd.setCursor(0, 2);
      lcd.print("> Night: ");
      if(NIGHT<10){
        lcd.print("0");
      }
      lcd.print(NIGHT);
      if (digitalRead(selectPin) == LOW) {
        delay(debounceDelay);
        if (digitalRead(selectPin) == LOW) {
          Serial.println("Select Pressed");
          bool run2it = true;
          while (run2it) {
            lcd.setCursor(0, 2);
            lcd.print("* Night: ");
            if(NIGHT < 10){
              lcd.print("0");
            }
            lcd.print(NIGHT);
            if (digitalRead(upPin) == LOW) {
              delay(debounceDelay);
              if (digitalRead(upPin) == LOW && NIGHT < 24) {
                Serial.println("Up Pressed");
                NIGHT++;
              } else if (digitalRead(upPin) == LOW && NIGHT >= 24) {
                Serial.println("Up Pressed");
                NIGHT = 0;
              }
            }

            if (digitalRead(downPin) == LOW) {
              delay(debounceDelay);
              if (digitalRead(downPin) == LOW && NIGHT > 0) {
                Serial.println("Down Pressed");
                NIGHT--;
              } else if (digitalRead(downPin) == LOW && NIGHT <= 0) {
                Serial.println("Down Pressed");
                NIGHT = 24;
              }
            }

            if (digitalRead(selectPin) == LOW) {
              Serial.println("Select Pressed");
              lcd.setCursor(0, 2);
              lcd.print("> Night: ");
              if(NIGHT < 10){
                lcd.print("0");
              }
              lcd.print(NIGHT);
              run2it = false;
            }
          }
        }
      }
    } else {
      lcd.setCursor(0, 2);
      lcd.print(" Night: ");
      if(NIGHT < 10){
        lcd.print("0");
      }
      lcd.print(NIGHT);
    }

    if (selectedMenuIndex == 3) {
      lcd.setCursor(0, 3);
      lcd.print("> RETURN");
      if (digitalRead(selectPin) == LOW) {
        delay(debounceDelay);
        if (digitalRead(selectPin) == LOW) {
          Serial.println("Select Pressed");
          currentMenu = STATS;
          break;
        }
      }
    } else {
      lcd.setCursor(0, 3);
      lcd.print("  RETURN");
    }
    menuNavigation();
  }
}

void ToggleLights() {
  delay(500);
  while (currentMenu == TOGGLE_LIGHTS) {
    if (digitalRead(LightRelay) == LOW) {
      digitalWrite(LightRelay, HIGH);  // Turn on the light relay if it's off
    } else {
      digitalWrite(LightRelay, LOW);   // Turn off the light relay if it's on
    }
    currentMenu = STATS;
    break;
  }
}

void SensorAdjust() {
  delay(500);
  while (currentMenu == SENSOR_ADJUST) {
    lcd.clear();
    
    while (digitalRead(selectPin) != LOW) {
      lcd.setCursor(0, 1);
      lcd.print("# of Sensors: ");
      lcd.print(numPins);
      
      if (digitalRead(upPin) == LOW){
        delay(debounceDelay);
        if (digitalRead(upPin) == LOW && numPins < maxPins) {
          numPins++;
        }
      }
      if (digitalRead(downPin) == LOW){
        delay(debounceDelay);
        if (digitalRead(downPin) == LOW && numPins > 0 ) {
          numPins--;
        }
      }
    }
    Serial.println("Select Pressed");
    EEPROM.write(EEPNumpins, numPins);
    currentMenu = STATS;
    break;
  }
}

void TogglePump() {
  delay(500);
  while (currentMenu == TOGGLE_PUMP) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WATERING");

    int remainingTime = wateringTime;
    int minutes, seconds;

    while (remainingTime > 0) {
      minutes = remainingTime / 60;
      seconds = remainingTime % 60;

      lcd.setCursor(0, 1);
      lcd.print("TIME LEFT: ");
      lcd.print(minutes);
      lcd.print(":");
      if (seconds < 10) {
        lcd.print(0);
      }
      lcd.print(seconds);

      if (digitalRead(selectPin) == LOW) {
        delay(debounceDelay);
        if (digitalRead(selectPin) == LOW) {
          Serial.println("Select Pressed");
          digitalWrite(PumpRelay, LOW);
          lcd.clear();
          lcd.setCursor(0,1);
          lcd.print("  WATERING STOPPED  ");
          delay(1000);
          currentMenu = MAIN_MENU;
          break;
        }
      }

      delay(1000);
      remainingTime--;
    }

    digitalWrite(PumpRelay, LOW); // Turn off the Pump
    Serial.println("PumpRelay turned off.");
    lcd.clear();
    currentMenu = STATS;
    break;
  }
}

void menuNavigation() {
  if (digitalRead(upPin) == LOW) {
    delay(debounceDelay);
    if (digitalRead(upPin) == LOW) {
      Serial.println("Up Pressed");
      selectedMenuIndex++;
      if (selectedMenuIndex > 3) {
        selectedMenuIndex = 1;
      }
    }
  }

  if (digitalRead(downPin) == LOW) {
    delay(debounceDelay);
    if (digitalRead(downPin) == LOW) {
      Serial.println("Down Pressed");
      selectedMenuIndex--;
      if (selectedMenuIndex < 1) {
        selectedMenuIndex = 3;
      }
    }
  }
}

void wateringTimeAdjust() {
  //ADD TO EEPROM
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Adjust Watering Time");
  while(currentMenu == WADJUST){
    lcd.setCursor(0,2);
    lcd.print("Time: ");
    lcd.print(wateringTime);
    if(digitalRead(upPin) == LOW){
      delay(debounceDelay);
      if(digitalRead(upPin) == LOW){
        wateringTime++;
      }
    } else if(digitalRead(downPin) == LOW){
      delay(debounceDelay);
      if(digitalRead(downPin) == LOW){
        wateringTime--;
      }
    }
    if(digitalRead(selectPin) == LOW){
      delay(debounceDelay);
      if(digitalRead(selectPin) == LOW){
        currentMenu = STATS;
        break;
      }
    }
  }
}

void getAverage() {
    for (int i = 0; i < numPins; i++) {
      pinValues[i] = analogRead(A0 + i);
    }

     // Calculate the average
    float sum = 0.0;
    for (int i = 0; i < numPins; i++) {
      sum += pinValues[i];
    }
    average = sum / numPins;
}