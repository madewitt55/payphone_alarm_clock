#include <TM1637Display.h>


const int dialPin = 2; // Triggers when rotary begins selection
const int pulsePin = 3; // Counts number of pulse to derive number
const int phoneLiftPin = 4; // Triggers when phone is lifted

// For controlling TM1637 display
const int CLK = 5;
const int DIO = 6;
TM1637Display display(CLK, DIO);

void setup() {
  pinMode(pulsePin, INPUT_PULLUP);
  pinMode(dialPin, INPUT_PULLUP);
  pinMode(phoneLiftPin, INPUT_PULLUP);
  display.setBrightness(0x0f);
  //display.showNumberDecEx(0, 0b01000000, true); // '0b01000000' enables colon

  Serial.begin(9600);
  Serial.println("Execution started:\n");
}

int dialState;
int pulseState;
int phoneLiftState;
int numberDialed;

int alarmTime[4] = {-1, -1, -1, -1}; // Holds 4 digits to make an alarm time, -1 = empty
uint8_t alarmTimeEncoded[4] = { 0x00, 0x00, 0x00, 0x00 }; // Holds encoded data for displaying time, 0x00 = blank
bool isAlarmSet = false;
int numDigits = 0; // Number of digits in time currently being entered

unsigned long currMillis = millis();
unsigned long prevMillis = millis(); // Used to store difference in time between animation phases
int animationDelay = 300; // 300ms
bool animationState = 0;

// Returns the number dialed on the rotary, -1 if error
int GetDialedNumber() {
  int numPulses = 0;
  int currentState = 1;

  while (dialState) {
    dialState = !digitalRead(dialPin);
    pulseState = !digitalRead(pulsePin);
    delay(10);

    // Flashing animation while dialing
    currMillis = millis();
    if (currMillis - prevMillis >= animationDelay) {
      prevMillis = currMillis;
      if (animationState) {
        display.showNumberDecEx(0, 0b01000000, true);
        animationState = 0;
      }
      else {
        display.clear();
        animationState = 1;
      }
    } 
    
    // If a change (pulse) has occured
    if (pulseState != currentState) {
      numPulses++;
      currentState = pulseState;
    }
  }

  if (numPulses) {
    if (numPulses == 20) { return 0; } // 0 is placed after 9 on rotary
    return numPulses / 2;
  }
  else {
    return -1;
  }
}

int AlarmTimeToInt(int time[4]) {
  String timeStr = "";
  for (int i = 0; i < 4; i++) {
    timeStr += time[i];
  }

  return timeStr.toInt();
}

bool IsValidTime(int time[4]) {
  if (time[0] * 10 + time[1] > 23) { return false; }
  else if (time[2] * 10 + time[3] > 59) { return false; }
  else { return true; }
}

void loop() {
  phoneLiftState = digitalRead(phoneLiftPin);
  Serial.println(phoneLiftState);
  
  // While phone is lifted
  while (phoneLiftState) {
    phoneLiftState = digitalRead(phoneLiftPin);
    dialState = !digitalRead(dialPin);
    display.setSegments(alarmTimeEncoded); // Display alarm time, whether partial or full, or blank if not set

    // If rotary begins to dial
    if (dialState) {
      // If alarm already set, unset it
      if (numDigits == 4) {
        for (int i = 0; i < 4; i++) {
          alarmTime[i] = -1;
          alarmTimeEncoded[i] = 0x00;
        }
        numDigits = 0;
        isAlarmSet = false;
      }

      Serial.println("Dialing...");
      numberDialed = GetDialedNumber();
      if (numberDialed != -1) {
        Serial.println("Number dialed: " + String(numberDialed));

        // Input number into full alarm time
        alarmTime[numDigits] = numberDialed;
        alarmTimeEncoded[numDigits] = display.encodeDigit(numberDialed);
        numDigits++;

        // If 4 digits, display with colon, else display partial time
        if (numDigits == 4) {
          display.showNumberDecEx(AlarmTimeToInt(alarmTime), 0b01000000, true);
        }
        else {
          display.setSegments(alarmTimeEncoded);
        }
      }
      else {
        Serial.println("Error getting number dialed");
      }
    }
  }

  if (numDigits == 4) {
    if (!isAlarmSet) {
      // Flash alarm time if valid
      if (IsValidTime(alarmTime)) {
        for (int i = 0; i < 5; i++) {
          display.showNumberDecEx(AlarmTimeToInt(alarmTime), 0b01000000, true);
          delay(animationDelay);
          display.clear();
          delay(animationDelay);
        }
        isAlarmSet = true;
      }
      // Flash zeroes if invalid time
      else {
        for (int i = 0; i < 5; i++) {
          display.showNumberDecEx(0, 0b01000000, true);
          delay(animationDelay);
          display.clear();
          delay(animationDelay);
        }
        // Clear invalid alarm time
        for (int i = 0; i < 4; i++) {
            alarmTime[i] = -1;
            alarmTimeEncoded[i] = 0x00;
        }
        numDigits = 0;
      }
    }
  }
  else {
    // Clear incomplete alarm time
    for (int i = 0; i < 4; i++) {
      alarmTime[i] = -1;
      alarmTimeEncoded[i] = 0x00;
    }
    numDigits = 0;
  }

  display.clear(); // Clear display when phone is placed
}