#include <Arduino.h>
#include <RTClib.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET    -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C

/*
  AUTOMATED EBB AND FLOW HYDROPONIC SYSTEM
  CONFIGURABLE WITH NANO & UNO
  WRITTEN BY CEDRIC MCENROE
*/

//---------Initialize I2C Devices----------
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
RTC_DS1307 rtc;

//----------Set Global Variables-----------
// Arduino Uno / Nano
uint8_t numPins = 2;            //Default Number of Sensors
#define maxPins 4           //Maximum Number of Sensors
/*
UNO / NANO PINOUT:
A0 - A3  -> Moisture Sensor
SDA / A4 -> RTC SDA
SCL / A5 -> RTC SCL
SDA / A4 -> OLED SDA
SCL / A5 -> OLED SCL
SDA / A4 -> TEMP SDA
SCL / A5 -> TEMP SCL
D2       -> UP Button
D3       -> DOWN Button
D5       -> SELECT Button
D10      -> Light Relay Signal
D11      -> Pump Relay Signal
*/

float average = 0.0;        // Sensor Average

#define LightRelay 10       // Digital pin connected to the Light Relay
#define PumpRelay 11        // Digital pin connected to the Pump Relay
#define upPin 2             // Digital pin connected to Up Button
#define downPin 3           // Digital pin connected to Down Button
#define selectPin 5         // Digital pin connected to Select Button
#define debounceDelay 80    // Adjust the delay value as needed
#define EEPNumpins 0        // numPins       EEPROM address
#define eepDay 1            // DAY           EEPROM address
#define eepNight 2          // NIGHT         EEPROM address
#define eepWadjust 4        // Watering Time EEPROM address
#define eepInterval 5       // Interval      EEPROM address
#define eepMethod 6         // Method        EEPROM address

uint8_t firstRun = 1;       // Bypass watering interval for first run
uint8_t DAY = 8;            // Lights turn on at this hour
uint8_t NIGHT = 20;         // Lights turn off at this hour
int* pinValues;             // Pointer to store pin values
uint8_t wateringTime = 255; // Watering time in seconds
uint8_t Interval = 6;       // Number of hours minimum between pump cycles
unsigned long startTime;    // Variable to store the start time of the timer
unsigned long timerDuration = Interval * 60 * 60 * 1000;  // 6 hours in milliseconds
uint8_t methodNumber = 0;

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
  WADJUST,
  INTERVALCHANGE,
  METHODCHANGE
};
MenuState currentMenu = STATS; //Default Menu State
int selectedMenuIndex = 1;

void setup() {
  Serial.begin(9600);
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  display.display();
  delay(2000); // Pause for 2 seconds

  // Clear the buffer
  display.clearDisplay();
  EEPROM.begin();
  Wire.begin();
  rtc.begin();
  pinValues = new int[maxPins]; // Dynamically allocate memory for pin values

  if (!rtc.isrunning()) {
    //Initial Setup (runs if clock is not set)
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    //clear all EEPROM values
    for (int i = 0 ; i < EEPROM.length() ; i++) {
      EEPROM.write(i, 0);
    }
    //set EEPROM to default variable values
    EEPROM.write(EEPNumpins, numPins);
    EEPROM.write(eepDay, DAY);
    EEPROM.write(eepNight, NIGHT);
    EEPROM.write(eepWadjust, wateringTime);
    EEPROM.write(eepInterval, Interval);
    EEPROM.write(eepMethod, methodNumber);
  }

  //set variables to EEPROM values
  numPins =      EEPROM.read(EEPNumpins);
  DAY =          EEPROM.read(eepDay);
  NIGHT =        EEPROM.read(eepNight);
  wateringTime = EEPROM.read(eepWadjust);
  Interval =     EEPROM.read(eepInterval);
  methodNumber = EEPROM.read(eepMethod);


  pinMode(LightRelay, OUTPUT);
  pinMode(PumpRelay, OUTPUT);
  pinMode(selectPin, INPUT_PULLUP);
  pinMode(upPin, INPUT_PULLUP);
  pinMode(downPin, INPUT_PULLUP);
}

void MainMenu() {
  delay(500);
  display.clearDisplay();
  while (currentMenu == MAIN_MENU) {
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println(F("Main Menu"));

    if (selectedMenuIndex == 1) {
      display.println(F("> Manual Control"));
      if (digitalRead(selectPin) == LOW) {
        delay(debounceDelay);
        if (digitalRead(selectPin) == LOW) {
          currentMenu = MANUAL_CONTROL;
        }
      }
    } else {
      display.println(F("  Manual Control"));
    }

    if (selectedMenuIndex == 2) {
      display.println(F("> Setup"));
      if (digitalRead(selectPin) == LOW) {
        delay(debounceDelay);
        if (digitalRead(selectPin) == LOW) {
          currentMenu = SETUP;
        }
      }
    } else {
      display.println(F("  Setup"));
    }

    if (selectedMenuIndex == 3) {
      display.println(F(""));
      display.println(F(""));
      display.println(F(""));
      display.println(F(""));
      display.println(F("> Return"));
      if (digitalRead(selectPin) == LOW) {
        delay(debounceDelay);
        if (digitalRead(selectPin) == LOW) {
          currentMenu = STATS;
        }
      }
    } else {
      display.println(F(""));
      display.println(F(""));
      display.println(F(""));
      display.println(F(""));
      display.println(F("  Return"));
    }
    display.display();
    menuNavigation();      //Menu logic, scrolls the cursor when the up and down buttons are pressed
  }
}

void SetupMenu() {
  delay(500);
  display.clearDisplay();
  while (currentMenu == SETUP) {
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println(F("Setup Menu"));

    if (selectedMenuIndex == 1) {
      display.println(F("> Configure Sensors"));
      if (digitalRead(selectPin) == LOW) {
        delay(debounceDelay);
        if (digitalRead(selectPin) == LOW) {
          currentMenu = SENSOR_ADJUST;
        }
      }
    } else {
      display.println(F("  Configure Sensors"));
    }

    if (selectedMenuIndex == 2) {
      display.println(F("> Adjust Lights"));
      if (digitalRead(selectPin) == LOW) {
        delay(debounceDelay);
        if (digitalRead(selectPin) == LOW) {
          currentMenu = LIGHT_ADJUST;
        }
      }
    } else {
      display.println(F("  Adjust Lights"));
    }

    if (selectedMenuIndex == 3) {
      display.println(F("> Watering Time"));
      if (digitalRead(selectPin) == LOW) {
        delay(debounceDelay);
        if (digitalRead(selectPin) == LOW) {
          currentMenu = WADJUST;
        }
      }
    } else {
      display.println(F("  Watering Time"));
    }

    if (selectedMenuIndex == 4) {
      display.println(F("> Watering Interval"));
      if (digitalRead(selectPin) == LOW) {
        delay(debounceDelay);
        if (digitalRead(selectPin) == LOW) {
          currentMenu = INTERVALCHANGE;
        }
      }
    } else {
      display.println(F("  Watering Interval"));
    }

    if (selectedMenuIndex == 5) {
      display.println(F("> Change Method"));
      if (digitalRead(selectPin) == LOW) {
        delay(debounceDelay);
        if (digitalRead(selectPin) == LOW) {
          currentMenu = METHODCHANGE;
        }
      }
    } else {
      display.println(F("  Change Method"));
    }

    if (selectedMenuIndex == 6) {
      display.println(F("> Return"));
      if (digitalRead(selectPin) == LOW) {
        delay(debounceDelay);
        if (digitalRead(selectPin) == LOW) {
          currentMenu = MAIN_MENU;
        }
      }
    } else {
      display.println(F("  Return"));
    }
    display.display();

    if (digitalRead(upPin) == LOW) {
      delay(debounceDelay);
      if (digitalRead(upPin) == LOW) {
        selectedMenuIndex++;
        if (selectedMenuIndex > 6) {
          selectedMenuIndex = 1;
        }
      }
    }

    if (digitalRead(downPin) == LOW) {
      delay(debounceDelay);
      if (digitalRead(downPin) == LOW) {
        selectedMenuIndex--;
        if (selectedMenuIndex < 1) {
          selectedMenuIndex = 6;
        }
      }
    }
  }
}

void ManualMenu() {
  delay(500);
  display.clearDisplay();
  while (currentMenu == MANUAL_CONTROL) {
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println(F("Manual Control"));
  
    if (selectedMenuIndex == 1) {
      display.println(F("> Toggle Lights"));
      if (digitalRead(selectPin) == LOW) {
        delay(debounceDelay);
        if (digitalRead(selectPin) == LOW) {
          currentMenu = TOGGLE_LIGHTS;
        }
      }
    } else {
      display.println(F("  Toggle Lights"));
    }
  
    if (selectedMenuIndex == 2) {
      display.println(F("> Toggle Pump"));
      if (digitalRead(selectPin) == LOW) {
        delay(debounceDelay);
        if (digitalRead(selectPin) == LOW) {
          currentMenu = TOGGLE_PUMP;
        }
      }
    } else {
      display.println(F("  Toggle Pump"));
    }
  
    if (selectedMenuIndex == 3) {
      display.println("");
      display.println("");
      display.println("");
      display.println("");
      display.println(F("> Return"));
      if (digitalRead(selectPin) == LOW) {
        delay(debounceDelay);
        if (digitalRead(selectPin) == LOW) {
          currentMenu = MAIN_MENU;
        }
      }
    } else {
      display.println(F(""));
      display.println(F(""));
      display.println(F(""));
      display.println(F(""));
      display.println(F("  Return"));
    }
    display.display();
    menuNavigation();
  }
}

void LightAdjust() {
  delay(500);
  display.clearDisplay();
  while (currentMenu == LIGHT_ADJUST) {
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println(F("Adjust Lighting Time"));

    if (selectedMenuIndex == 1) {
      display.print(F("> Day: "));
      if(DAY<10){
        display.print(0);
      }
      display.println(DAY);
      if (digitalRead(selectPin) == LOW) {
        delay(debounceDelay);
        if (digitalRead(selectPin) == LOW) {
          bool runit = true;
          while (runit) {
            display.print(F("* Day: "));
            if(DAY<10){
              display.print("0");
            }
            display.println(DAY);
            if (digitalRead(upPin) == LOW) {
              delay(debounceDelay);
              if (digitalRead(upPin) == LOW && DAY < 24) {
                DAY++;
              } else if (digitalRead(upPin) == LOW && DAY >= 24) {
                DAY = 0;
              }
            }

            if (digitalRead(downPin) == LOW) {
              delay(debounceDelay);
              if (digitalRead(downPin) == LOW && DAY > 0) {
                DAY--;
              } else if (digitalRead(downPin) == LOW && DAY <= 0) {
                DAY = 24;
              }
            }

            if (digitalRead(selectPin) == LOW) {
              display.print(F("> Day: "));
              if(DAY < 10){
                display.print("0");
              }
              display.println(DAY);
              runit = false;
            }
          }
        }
      }
    } else {
      display.print(F("  Day: "));
      if(DAY < 10){
        display.print(F("0"));
      }
      display.println(DAY);
    }

    if (selectedMenuIndex == 2) {
      display.print(F("> Night: "));
      if(NIGHT<10){
        display.print(F("0"));
      }
      display.println(NIGHT);
      if (digitalRead(selectPin) == LOW) {
        delay(debounceDelay);
        if (digitalRead(selectPin) == LOW) {
          bool run2it = true;
          while (run2it) {
            display.print(F("* Night: "));
            if(NIGHT < 10){
              display.print(F("0"));
            }
            display.println(NIGHT);
            if (digitalRead(upPin) == LOW) {
              delay(debounceDelay);
              if (digitalRead(upPin) == LOW && NIGHT < 24) {
                NIGHT++;
              } else if (digitalRead(upPin) == LOW && NIGHT >= 24) {
                NIGHT = 0;
              }
            }

            if (digitalRead(downPin) == LOW) {
              delay(debounceDelay);
              if (digitalRead(downPin) == LOW && NIGHT > 0) {
                NIGHT--;
              } else if (digitalRead(downPin) == LOW && NIGHT <= 0) {
                NIGHT = 24;
              }
            }

            if (digitalRead(selectPin) == LOW) {
              display.print(F("> Night: "));
              if(NIGHT < 10){
                display.print(F("0"));
              }
              display.println(NIGHT);
              run2it = false;
            }
          }
        }
      }
    } else {
      display.print(F(" Night: "));
      if(NIGHT < 10){
        display.print(F("0"));
      }
      display.println(NIGHT);
    }

    if (selectedMenuIndex == 3) {
      display.println("");
      display.println("");
      display.println("");
      display.println("");
      display.print(F("> RETURN"));
      if (digitalRead(selectPin) == LOW) {
        delay(debounceDelay);
        if (digitalRead(selectPin) == LOW) {
          EEPROM.write(eepDay, DAY);
          EEPROM.write(eepNight, NIGHT);
          currentMenu = STATS;
        }
      }
    } else {
      display.println("");
      display.println("");
      display.println("");
      display.println("");
      display.print(F("  RETURN"));
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
  }
}

void SensorAdjust() {
  delay(500);
  display.clearDisplay();
  while (currentMenu == SENSOR_ADJUST) {
    while (digitalRead(selectPin) != LOW) {
      display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(0, 24);
      display.print(F("# of Sensors: "));
      display.print(numPins);
      display.display();
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
    EEPROM.write(EEPNumpins, numPins);
    currentMenu = STATS;
  }
}

void TogglePump() {
  display.clearDisplay();
  while (currentMenu == TOGGLE_PUMP) {

    int remainingTime = wateringTime;
    int minutes, seconds;

    while (remainingTime > 0) {
      display.setTextSize(2);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(0, 0);
      display.println(F(" WATERING"));

      minutes = remainingTime / 60;
      seconds = remainingTime % 60;

      display.println(F(""));
      display.setTextSize(3);
      display.print(F(" "));
      display.print(minutes);
      display.print(":");
      if (seconds < 10) {
        display.print(0);
      }
      display.println(seconds);

      if (digitalRead(selectPin) == LOW) {
        delay(debounceDelay);
        if (digitalRead(selectPin) == LOW) {
          digitalWrite(PumpRelay, LOW);
          display.clearDisplay();
          display.setTextSize(2);
          display.setTextColor(SSD1306_WHITE);
          display.setCursor(0, 0);
          display.println(F(""));
          display.println(F("WATERING"));
          display.println(F("STOPPED"));
          delay(1000);
          currentMenu = MAIN_MENU;
        }
      }
      display.display();
      delay(1000);
      display.clearDisplay();
      remainingTime--;
    }
    //Turn off the Pump
    digitalWrite(PumpRelay, LOW); 
    currentMenu = STATS;
  }
}

void menuNavigation() {
  if (digitalRead(upPin) == LOW) {
    delay(debounceDelay);
    if (digitalRead(upPin) == LOW) {
      selectedMenuIndex++;
      if (selectedMenuIndex > 3) {
        selectedMenuIndex = 1;
      }
    }
  }

  if (digitalRead(downPin) == LOW) {
    delay(debounceDelay);
    if (digitalRead(downPin) == LOW) {
      selectedMenuIndex--;
      if (selectedMenuIndex < 1) {
        selectedMenuIndex = 3;
      }
    }
  }
}

void wateringTimeAdjust() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println(F("Adjust Watering Time"));
  while(currentMenu == WADJUST){
    display.setCursor(0,16);
    display.print(F("Seconds: "));
    display.print(wateringTime);
    display.display();
    if(digitalRead(upPin) == LOW){
      delay(debounceDelay);
      if(digitalRead(upPin) == LOW && wateringTime < 255){
        wateringTime++;
      }
    } else if(digitalRead(downPin) == LOW){
      delay(debounceDelay);
      if(digitalRead(downPin) == LOW && wateringTime > 0){
        wateringTime--;
      }
    }
    if(digitalRead(selectPin) == LOW){
      delay(debounceDelay);
      if(digitalRead(selectPin) == LOW){
        EEPROM.write(eepWadjust, wateringTime);
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

void autoLight() {
  DateTime now = rtc.now();
  int hour = now.hour();
  int minute = now.minute();
  int second = now.second();
  if (now.hour() >= DAY && now.hour() < NIGHT) {
    digitalWrite(LightRelay, HIGH);
  } else {
    digitalWrite(LightRelay, LOW);
  }
}

void selectMethod() {
  display.clearDisplay();
  while(currentMenu == METHODCHANGE){
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0,0);
    display.println(F("CHANGE METHOD"));
    if (selectedMenuIndex == 1){
      display.println(F("> EBB & FLOW"));
      if (digitalRead(selectPin) == LOW){
        delay(debounceDelay);
        if(digitalRead(selectPin) == LOW){
          display.clearDisplay();
          display.setTextSize(1);
          display.setTextColor(SSD1306_WHITE);
          display.setCursor(0,24);
          display.print(F("Selected EBB & FLOW"));
          methodNumber = 0;
          delay(2000);
          EEPROM.write(eepMethod, methodNumber);
          currentMenu = STATS;
        }
      }
    } else {
      display.println(F("  EBB & FLOW"));
    }
    if (selectedMenuIndex == 2){
      display.println(F("> MYCELIUM"));
      if(digitalRead(selectPin) == LOW){
        delay(debounceDelay);
        if(digitalRead(selectPin) == LOW){
          display.clearDisplay();
          display.setTextSize(1);
          display.setTextColor(SSD1306_WHITE);
          display.setCursor(0,24);
          display.print(F("Selected MYCELIUM"));
          methodNumber = 2;
          delay(2000);
          EEPROM.write(eepMethod, methodNumber);
          currentMenu = STATS;
        }
      }
    }
    if (selectedMenuIndex == 3){
      display.println(F("> DEEP WATER CULTURE"));
      if(digitalRead(selectPin) == LOW){
        delay(debounceDelay);
        if(digitalRead(selectPin) == LOW){
          display.clearDisplay();
          display.setTextSize(1);
          display.setTextColor(SSD1306_WHITE);
          display.setCursor(0,24);
          display.print(F("Selected D.W.C."));
          methodNumber = 4;
          delay(2000);
          EEPROM.write(eepMethod, methodNumber);
          currentMenu = STATS;
        }
      }
    }
  }
    if (selectedMenuIndex == 4){
      display.println(F("> Wick"));
      if(digitalRead(selectPin) == LOW){
        delay(debounceDelay);
        if(digitalRead(selectPin) == LOW){
          display.clearDisplay();
          display.setTextSize(1);
          display.setTextColor(SSD1306_WHITE);
          display.setCursor(0,24);
          display.print(F("Selected Wick"));
          methodNumber = 3;
          delay(2000);
          EEPROM.write(eepMethod, methodNumber);
          currentMenu = STATS;
        }
      }
    }
    if (selectedMenuIndex == 5){
      display.println(F("> Nutrient Film"));
      if(digitalRead(selectPin) == LOW){
        delay(debounceDelay);
        if(digitalRead(selectPin) == LOW){
          display.clearDisplay();
          display.setTextSize(1);
          display.setTextColor(SSD1306_WHITE);
          display.setCursor(0,24);
          display.print(F("Selected N.F."));
          methodNumber = 1;
          delay(2000);
          EEPROM.write(eepMethod, methodNumber);
          currentMenu = STATS;
        }
      }
    }
    if (selectedMenuIndex == 6){
      display.println("");
      display.println(F("> Return"));
      if(digitalRead(selectPin) == LOW){
        delay(debounceDelay);
        if(digitalRead(selectPin) == LOW){
          currentMenu = STATS;
        }
      }
    }
    if (digitalRead(upPin) == LOW) {
      delay(debounceDelay);
      if (digitalRead(upPin) == LOW) {
        selectedMenuIndex++;
          if (selectedMenuIndex > 6) {
            selectedMenuIndex = 1;
          }
      }
    }
    if (digitalRead(downPin) == LOW) {
      delay(debounceDelay);
      if (digitalRead(downPin) == LOW) {
        selectedMenuIndex--;
        if (selectedMenuIndex < 1) {
          selectedMenuIndex = 6;
        }
      }
    }
}


void waterInterval() {
  display.clearDisplay();
  while (currentMenu == INTERVALCHANGE) {
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println(F("Hours Between Cycles"));
    display.setCursor(0, 24);
    display.print(F("Interval = "));
    display.print(Interval);
    display.println(" HRS");
    display.display();
    if (digitalRead(upPin) == LOW) {
      delay(debounceDelay);
      if (digitalRead(upPin) == LOW) {
        Interval++;
      }
    }
    if (digitalRead(downPin) == LOW) {
      delay(debounceDelay);
      if (digitalRead(downPin) == LOW) {
        Interval--;
      }
    }
    if (digitalRead(selectPin) == LOW) {
      delay(debounceDelay);
      if (digitalRead(selectPin) == LOW) {
        EEPROM.write(eepInterval, Interval);
        currentMenu = STATS;
      }
    }
  }
}

bool isTimerElapsed() {
  unsigned long elapsedTime = millis() - startTime;  // Calculate the elapsed time

  if (elapsedTime >= timerDuration) {
    return true;  // Timer has elapsed
  } else {
    return false;  // Timer is still running
  }
}

void startTimer() {
  startTime = millis();  // Record the current time as the start time
  firstRun = 0;
}

void DisplayStats() {
  display.clearDisplay();
  while (currentMenu == STATS) {
    if (digitalRead(selectPin) == LOW) {
      delay(debounceDelay);
      if (digitalRead(selectPin) == LOW) {
        currentMenu = MAIN_MENU;
      }
    }

    DateTime now = rtc.now();     //Pulling the time from the RTC module each loop to keep in sync
    int hour = now.hour();
    int minute = now.minute();
    int second = now.second();

    autoLight();                  //call autoLight(); to see if lights need to be turned on or off
    getAverage();                 //call getAverage(); to do read sensor values and average them out

    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println(F(" MOISTURE"));  //print to allow something else to print on the same line
    display.print(average);

    if (average <= 400) {
      display.println(" WET");    //println to move the following to a new line
    } else if (average > 400) {
      display.println(" DRY");    //println to move the following to a new line
    }
    display.println("");
    display.print("TIME ");
    display.print(now.hour(), DEC);
    display.print(":");

    if (now.minute() < 10) {     //Logic to print a 0 following single digit seconds
      display.print(0);
    }
    display.println(now.minute(), DEC);

    if (average >= 400) {
      if(isTimerElapsed()){
        startTimer();
        currentMenu = TOGGLE_PUMP;
      } else if (firstRun == 1){
        startTimer();
        currentMenu = TOGGLE_PUMP;
      }
    }
    display.display();           //Flashes everything on display for 250 milliseconds before updating
    delay(250);
    display.clearDisplay();
  }
}

void loop() {
  //Menu Switch
  switch (currentMenu) {
    case MAIN_MENU:
      MainMenu();
    case MANUAL_CONTROL:
      ManualMenu();
    case SETUP:
      SetupMenu();
    case STATS:
      DisplayStats();
    case SENSOR_ADJUST:
      SensorAdjust();
    case LIGHT_ADJUST:
      LightAdjust();
    case TOGGLE_LIGHTS:
      ToggleLights();
    case TOGGLE_PUMP:
      TogglePump();
    case WADJUST:
      wateringTimeAdjust();
    case INTERVALCHANGE:
      waterInterval();
    case METHODCHANGE:
      selectMethod();
  }
}
