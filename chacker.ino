//Include I2C LCD libraries.
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

//Define the pins.
#define mosfetPin 2
#define goPin 3
#define stopPin 4
#define batteryVoltagePin A0 

LiquidCrystal_I2C lcd(0x27, 16, 2);

bool runLoad = false;
long lastMillis = 0;
long lastMillisPoll = 0;
long lastMillisLCD = 0;

bool goPinLastState = HIGH;
bool stopPinLastState = HIGH;

const float systemVoltage = 5.01; //Enter system voltage here.
float vpc = systemVoltage / 1024;

const float voltageDivider1 = 9.66; //Put R1 resistance here.
const float voltageDivider2 = 4.95; //Put R2 resistance here.

float multiplier = (voltageDivider1 + voltageDivider2) / voltageDivider2;

const float loadResistance = 7.51; //Put R3 and fan equivalent resistance here.

float ampSecondsSum = 0;
int ampSecondsCount = 0;

float batteryVoltageSum = 0;
int batteryVoltageCount = 0;
float batteryVoltageAvg = 0;

long millisStarted = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("Starting Up!");

  Serial.println("Multiplier: " + String(multiplier));

  pinMode(mosfetPin, OUTPUT);
  pinMode(goPin, INPUT_PULLUP);
  pinMode(stopPin, INPUT_PULLUP);
  pinMode(batteryVoltagePin, INPUT);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Starting Up!");
  delay(1000);
  lcd.setCursor(0, 0);
  lcd.print("Press to start!");
}

void loop() {
  // put your main code here, to run repeatedly:

  long millisNow = millis();

  if ((digitalRead(goPin) != goPinLastState) && digitalRead(goPin) == LOW && runLoad == false) {
    Serial.println("Go!");

    millisStarted = 0;
    ampSecondsSum = 0;
    ampSecondsCount = 0;

    runLoad = true;
    lcd.clear();
    digitalWrite(mosfetPin, HIGH);
    millisStarted = millisNow;
    Serial.println("Mosfet High!");
  }
  goPinLastState = digitalRead(goPin);

  if ((digitalRead(stopPin) != stopPinLastState) && digitalRead(stopPin) == LOW && runLoad == true) {
    Serial.println("Stop!");
    runLoad = false;
    digitalWrite(mosfetPin, LOW);
  }
  stopPinLastState = digitalRead(stopPin);

  if (runLoad) {
    if (millisNow >= lastMillisPoll + 100) {

      int counts = analogRead(batteryVoltagePin);
      float voltageIn = counts * vpc;
      Serial.println("Voltage In: " + String(voltageIn));
      batteryVoltageSum += (voltageIn * multiplier);
      batteryVoltageCount++;

      lastMillisPoll = millisNow;
    }

    if (millisNow >= lastMillisLCD + 1000) {
      Serial.println("Time to do some logging!");
      long seconds = ((millisNow - millisStarted) / 1000);

      long minutes = seconds / 60;
      seconds -= minutes * 60;

      long hours = minutes / 60;
      minutes -= hours * 60;

      char duration[9];
      sprintf(duration, "%02ld:%02ld:%02ld", hours, minutes, seconds);

      Serial.println("Duration: " + String(duration));
      lcd.setCursor(0, 0);
      lcd.print(duration);

      lcd.setCursor(10, 1);

      batteryVoltageAvg = batteryVoltageSum / batteryVoltageCount;

      batteryVoltageSum = 0;
      batteryVoltageCount = 0;

      lcd.print(String(batteryVoltageAvg) + "V");
      float amperage = batteryVoltageAvg / loadResistance;
      Serial.println("Amperage: " + String(amperage));
      lcd.setCursor(0, 1);
      lcd.print(String(amperage) + "A");

      ampSecondsSum += amperage;
      Serial.println("ampSecondsSum: " + String(ampSecondsSum));
      ampSecondsCount++;
      Serial.println("ampSecondsCount: " + String(ampSecondsCount));

      String ampHours = String(ampSecondsSum / 3600) + "Ah";

      Serial.println("Ah: " + ampHours);

      lcd.setCursor(16 - ampHours.length(), 0);
      lcd.print(ampHours);

      lastMillisLCD = millisNow;
    }

    if (batteryVoltageAvg < 12) {
      runLoad = false;
      digitalWrite(mosfetPin, LOW);
      millisStarted = 0;
      Serial.println("Battery Discharged!");
    }
  }

  delay(1);
}
