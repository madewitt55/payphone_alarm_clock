#include <TM1637Display.h>
#include "Time.h" // Class for managing time

// INPUT
const int dialPin = 2; // Triggers when rotary begins selection
const int pulsePin = 3; // Counts number of pulse to derive number
const int phoneLiftPin = 4; // Triggers when phone is lifted

// OUTPUT
const int bellPin = 7; // Spins motor to ring bell

// TM1637 Display
const int CLK = 5;
const int DIO = 6;
TM1637Display display(CLK, DIO);


Time initTime = Time(12, 0); // Set inital time to 12:00
Time currTime = Time(millis(), initTime); // Set current time (can be changed later)
Time alarm = Time(12, 0); // Dummy alarm time (will not go off until manually set by user)

void setup() {
  pinMode(pulsePin, INPUT_PULLUP);
  pinMode(dialPin, INPUT_PULLUP);
  pinMode(phoneLiftPin, INPUT_PULLUP);
  pinMode(bellPin, OUTPUT);
  display.setBrightness(0x0f);

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
int ringDelay = 600;
bool animationState = 0;

// Counts pulses and returns the number that was input to rotary, -1 if error
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
        display.showNumberDecEx(0, 0b01000000, true); // Flash zeroes
        animationState = 0;
      }
      else {
        display.clear(); // Flash blank
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

// Converts array of 4 digits to integer
// Used for outputting full time to display
int AlarmTimeToInt(int time[4]) {
  String timeStr = "";
  for (int i = 0; i < 4; i++) {
    timeStr += time[i];
  }

  return timeStr.toInt();
}

// Checks if array of 4 digits is a valid time to set alarm to
bool IsValidTime(int time[4]) {
  if (time[0] * 10 + time[1] > 23) { return false; }
  else if (time[2] * 10 + time[3] > 59) { return false; }
  else { return true; }
}

void loop() {
  phoneLiftState = digitalRead(phoneLiftPin);
  
  // While phone is lifted
  while (phoneLiftState) {
    phoneLiftState = digitalRead(phoneLiftPin);
    dialState = !digitalRead(dialPin);
    display.setSegments(alarmTimeEncoded); // Display alarm time, blank if not set

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
        alarm.SetHour(alarmTime[0] * 10 + alarmTime[1]);
        alarm.SetMinute(alarmTime[2] * 10 + alarmTime[3]);
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

  currTime.SetTimeMillis(millis(), initTime); // Update current time
  // When alarm time is reached
  if (isAlarmSet && currTime == alarm) {
    // Lift phone to turn off alarm
    while(!phoneLiftState) {
      phoneLiftState = digitalRead(phoneLiftPin);

      // Flashing animation and ringing bell
      currMillis = millis();
      if (currMillis - prevMillis >= ringDelay) {
        prevMillis = currMillis;
        if (animationState) {
          display.showNumberDecEx(0, 0b01000000, true); // Flash zeroes
          digitalWrite(bellPin, HIGH); // Turn bell on
          animationState = 0;
        }
        else {
          display.clear(); // Flash blank
          digitalWrite(bellPin, LOW); // Turn bell off
          animationState = 1;
        }
      } 
    }

    // Clear alarm time
    for (int i = 0; i < 4; i++) {
        alarmTime[i] = -1;
        alarmTimeEncoded[i] = 0x00;
    }
    numDigits = 0;
    isAlarmSet = false;
  }

  display.clear(); // Clear display when phone is placed
}
