const int pulsePin = 3; // Counts number of pulse to derive number
const int dialPin = 2; // Triggers when rotary begins selection
const int phoneLiftPin = 4;

void setup() {
  pinMode(pulsePin, INPUT_PULLUP);
  pinMode(dialPin, INPUT_PULLUP);
  pinMode(phoneLiftPin, INPUT_PULLUP);
  Serial.begin(9600);
  Serial.println("Execution started:\n");
}

int dialState;
int pulseState;
int phoneLiftState;
int numberDialed;
int alarmTime[4];
int currIndex = 0;

// Returns the number dialed on the rotary
int GetDialedNumber() {
  int numPulses = 0;
  int currentState = 1;

  while (dialState) {
    dialState = !digitalRead(dialPin);
    pulseState = !digitalRead(pulsePin);
    delay(10);
    
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
}

void loop() {
  phoneLiftState = digitalRead(phoneLiftPin);

  while (phoneLiftState) {
    phoneLiftState = digitalRead(phoneLiftPin);
    dialState = !digitalRead(dialPin);

  if (dialState && currIndex < 4) {
    Serial.println("Dialing...");
    numberDialed = GetDialedNumber();
    Serial.println("Number dialed: " + String(numberDialed));

    // Input number into full alarm time
    alarmTime[currIndex] = numberDialed;
    currIndex++;
  }

  // If alarm time is complete
  if (currIndex == 4) {
    Serial.print("Alarm set for: ");
    for (int i = 0; i < 4; i++) {
      Serial.print(String(alarmTime[i]));
      if (i == 1) {
        Serial.print(":");
      }
    }
    currIndex = 0;
  }
  }
}