#include <TM1637Display.h>
#include<math.h>

// INPUT
const int DIAL_PIN = 2; // Triggers when rotary begins selection
const int PULSE_PIN = 3; // Counts number of pulse to derive number
const int PHONE_LIFT_PIN = 4; // Triggers when phone is lifted

// OUTPUT
const int BELL_PIN = 7; // Spins motor to ring bell

// Input codes to change alarm clock settings
const int CLOCK_FORMAT_CODE = 2500;
const int HIDE_TIME_CODE = 2600;
const int CURR_TIME_CODE = 2700;
const int MATH_MODE_CODE = 2800;

int dial_state;
int pulse_state;
int phone_lift_state;
int number_dialed;

// TM1637 Display
const int CLK = 5;
const int DIO = 6;
TM1637Display display(CLK, DIO);

// Current time tracking
int initial_time = 1745; // Last updated system time
unsigned long initial_millis = millis(); // Resets when inital time is updated; used to calculate current time
bool is_12hr; // Is clock set to 12 hour format

// For setting alarm time
int alarm_time = 0;
String alarm_str = "";
uint8_t alarmTimeEncoded[4] = { 0x00, 0x00, 0x00, 0x00 }; // Holds encoded data for displaying time, 0x00 = blank
bool is_alarm_set = false;
int num_digits = 0; // Number of digits in currently stored alarm time

// For setting new system time
int new_curr_time = 0;
String new_curr_time_str = "";
uint8_t new_curr_time_encoded[4] = { 0x00, 0x00, 0x00, 0x00 };
int num_new_curr_time_digits = 0;

// For animations and timing
unsigned long curr_millis = millis();
unsigned long prev_millis = millis();
int animation_delay = 300; // ms
bool animation_state = 0;
int ring_delay = 600;
int hold_duration = 10000; // Dialed must be held for 10s to set system time

void setup() {
  pinMode(PULSE_PIN, INPUT_PULLUP);
  pinMode(DIAL_PIN, INPUT_PULLUP);
  pinMode(PHONE_LIFT_PIN, INPUT_PULLUP);
  pinMode(BELL_PIN, OUTPUT);
  display.setBrightness(0x0f);

  Serial.begin(9600);
  Serial.println("Execution started:\n");
}

// Returns current time as an integer
int GetCurrTime() {
  unsigned long millis_passed = millis() - initial_millis;
  long seconds_passed = millis_passed / 1000;
  int hours_passed = seconds_passed / 3600;
  int minutes_passed = (seconds_passed % 3600) / 60;

  int initial_hours = initial_time / 100;
  int initial_minutes = initial_time % 100;

  int total_minutes = initial_minutes + minutes_passed;
  int total_hours = initial_hours + hours_passed + (total_minutes / 60);
  total_minutes %= 60; // Normalize minutes
  total_hours %= 24;   // Normalize hours

  return total_hours * 100 + total_minutes;
}

// Counts pulses and returns the number that was input to rotary, -1 if error
int GetDialedNumber() {
  int num_pulses = 0;
  int current_state = 1;

  while (dial_state) {
    dial_state = !digitalRead(DIAL_PIN);
    pulse_state = !digitalRead(PULSE_PIN);
    delay(10);

    // Flashing animation while dialing
    curr_millis = millis();
    if (curr_millis - prev_millis >= animation_delay) {
      prev_millis = curr_millis;
      if (animation_state) {
        display.showNumberDecEx(0, 0b01000000, true); // Flash zeroes
        animation_state = 0;
      }
      else {
        display.clear(); // Flash blank
        animation_state = 1;
      }
    } 
    
    // If a change (pulse) has occured
    if (pulse_state != current_state) {
      num_pulses++;
      current_state = pulse_state;
    }
  }

  if (num_pulses) {
    if (num_pulses == 20) { return 0; } // 0 is placed after 9 on rotary
    else { return num_pulses / 2; }
  }
  else {
    return -1;
  }
}

// Checks if an integer is a valid time
bool IsValidTime(int time) {
  if (time / 100 >= 24 || time % 100 >= 60 || time / 100 == 0) {
    return false;
  }
  else {
    return true;
  }
}

void loop() {
  display.showNumberDecEx(GetCurrTime(), 0b01000000, true); // Display current time

  // While phone is lifted
  phone_lift_state = digitalRead(PHONE_LIFT_PIN);
  while (phone_lift_state) {
    phone_lift_state = digitalRead(PHONE_LIFT_PIN);
    dial_state = !digitalRead(DIAL_PIN);

    // Display alarm time, blank for unset
    if (is_alarm_set) {
      display.showNumberDecEx(alarm_time, 0b01000000, true);
    }
    else {
      display.setSegments(alarmTimeEncoded);
    }

    // If rotary begins to dial
    if (dial_state) {
      // If alarm already set, unset it
      if (is_alarm_set) {
        for (int i = 0; i < 4; i++) {
          alarmTimeEncoded[i] = 0x00;
        }
        alarm_time = 0;
        alarm_str = "";
        num_digits = 0;
        is_alarm_set = false;
      }

      number_dialed = GetDialedNumber();
      if (number_dialed != -1) {
        // Input number into full alarm time
        alarm_str += number_dialed;
        alarm_time = alarm_str.toInt();
        alarmTimeEncoded[num_digits] = display.encodeDigit(number_dialed);
        num_digits++;
      }
    }
  }

  // When phone is replaced
  if (!is_alarm_set && num_digits > 0) {
    // Valid input
    if (IsValidTime(alarm_time)) {
      for (int i = 0; i < 5; i++) {
        display.showNumberDecEx(alarm_time, 0b01000000, true);
        delay(animation_delay);
        display.clear();
        delay(animation_delay);
      }
      is_alarm_set = true;
    }
    // Invalid input
    else {
      for (int i = 0; i < 5; i++) {
        display.showNumberDecEx(0, 0b01000000, true);
        delay(animation_delay);
        display.clear();
        delay(animation_delay);
      }
      // Clear invalid alarm time
      for (int i = 0; i < 4; i++) {
          alarmTimeEncoded[i] = 0x00;
      }
      alarm_time = 0;
      alarm_str = "";
      num_digits = 0;
    }
  }

  // When alarm time is reached
  if (alarm_time == GetCurrTime() && is_alarm_set) {
    // Until phone is lifted
    while(!phone_lift_state) {
      phone_lift_state = digitalRead(PHONE_LIFT_PIN);

      // Flashing animation and ringing bell
      curr_millis = millis();
      if (curr_millis - prev_millis >= ring_delay) {
        prev_millis = curr_millis;
        if (animation_state) {
          display.showNumberDecEx(alarm_time, 0b01000000, true); // Flash alarm time (current time)
          digitalWrite(BELL_PIN, HIGH); // Bell on
          animation_state = 0;
        }
        else {
          display.clear(); // Flash blank
          digitalWrite(BELL_PIN, LOW); // Bell off
          animation_state = 1;
        }
      } 
    }
    digitalWrite(BELL_PIN, LOW); // Bell off

    // Clear alarm time
    for (int i = 0; i < 4; i++) {
        alarmTimeEncoded[i] = 0x00;
    }
    alarm_time = 0;
    alarm_str = "";
    num_digits = 0;
    is_alarm_set = false;
  }

  // Set system time mode
  dial_state = !digitalRead(DIAL_PIN);
  if (dial_state) {
    prev_millis = millis();
    while (dial_state) {
      dial_state = !digitalRead(DIAL_PIN);
      curr_millis = millis();
      // Dial held for 10 seconds
      if (curr_millis - prev_millis >= hold_duration) {
        // Play animation
        for (int i = 0; i < 5; i++) {
          display.showNumberDecEx(0, 0b01000000, true);
          delay(animation_delay);
          display.clear();
          delay(animation_delay);
        }
        phone_lift_state = digitalRead(PHONE_LIFT_PIN);

        // Phone is lifted (user has entered set time mode)
        while (phone_lift_state) {
          phone_lift_state = digitalRead(PHONE_LIFT_PIN);
          dial_state = !digitalRead(DIAL_PIN);
          display.setSegments(new_curr_time_encoded);

          if (dial_state) {
            number_dialed = GetDialedNumber();
            if (number_dialed != -1) {
              // Input number into full time
              new_curr_time_str += number_dialed;
              new_curr_time = new_curr_time_str.toInt();
              new_curr_time_encoded[num_new_curr_time_digits] = display.encodeDigit(number_dialed);
              num_new_curr_time_digits++;
            }
          }
        }

        // When phone is replaced
        if (num_new_curr_time_digits > 0) {
          // Valid input
          if (IsValidTime(new_curr_time)) {
            for (int i = 0; i < 5; i++) {
              display.showNumberDecEx(new_curr_time, 0b01000000, true);
              delay(animation_delay);
              display.clear();
              delay(animation_delay);
            }
            initial_time = new_curr_time;
            initial_millis = millis();
          }
          // Invalid input
          else {
            for (int i = 0; i < 5; i++) {
              display.showNumberDecEx(0, 0b01000000, true);
              delay(animation_delay);
              display.clear();
              delay(animation_delay);
            }
            // Clear invalid alarm time
            for (int i = 0; i < 4; i++) {
                new_curr_time_encoded[i] = 0x00;
            }
            new_curr_time = 0;
            new_curr_time_str = "";
            num_new_curr_time_digits = 0;
          }
        }
      }
    }
  }
}
