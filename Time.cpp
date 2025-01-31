#include "Time.h"

Time::Time(unsigned long millis, Time initTime) {
    unsigned long totalMins = millis / 60000;
    unsigned long totalHrs = totalMins / 60;

    // Add total hours and minutes separately to prevent overflow
    hour = initTime.GetHour() + totalHrs;
    minute = initTime.GetMinute() + totalMins % 60;  // Only add the remainder of the minutes

    // Handle minute overflow
    if (minute >= 60) {
        hour += minute / 60;  // Add overflow hours to hour
        minute = minute % 60; // Keep minute in 0-59 range
    }

    // Handle hour overflow (24-hour format)
    if (hour >= 24) {
        hour = hour % 24;  // Keep hour in 0-23 range
    }
}

Time::Time(int hr, int min) {
    hour = hr % 24;   // Ensure hour is always between 0 and 23
    minute = min % 60; // Ensure minute is always between 0 and 59
}

void Time::SetHour(int hr) {
  hour = hr;
}

int Time::GetHour() {
  return hour;
}

void Time::SetMinute(int min) {
  minute = min;
}

int Time::GetMinute() {
  return minute;
}

void Time::SetTimeMillis(unsigned long millis, Time initTime) {
    unsigned long totalMins = millis / 60000;
    unsigned long totalHrs = totalMins / 60;

    // Add total hours and minutes separately to prevent overflow
    hour = initTime.GetHour() + totalHrs;
    minute = initTime.GetMinute() + totalMins % 60;  // Only add the remainder of the minutes

    // Handle minute overflow
    if (minute >= 60) {
        hour += minute / 60;  // Add overflow hours to hour
        minute = minute % 60; // Keep minute in 0-59 range
    }

    // Handle hour overflow (24-hour format)
    if (hour >= 24) {
        hour = hour % 24;  // Keep hour in 0-23 range
    }
}

void Time::SetSysTime(unsigned long millis, int currHr, int currMin) {
    // Convert millis into hours and minutes
    unsigned long totalMins = millis / 60000; // Millis to minutes
    unsigned long totalHrs = totalMins / 60;  // Millis to hours
    totalMins = totalMins % 60; // Remaining minutes after hours conversion

    // Calculate the total hours and minutes since the system started
    int newHour = currHr + totalHrs;
    int newMinute = currMin + totalMins;

    // Handle minute overflow
    if (newMinute >= 60) {
        newMinute -= 60;
        newHour++;
    }

    // Handle hour overflow (24-hour format)
    if (newHour >= 24) {
        newHour -= 24;
    }

    // Update the current time object with the calculated values
    SetHour(newHour);
    SetMinute(newMinute);
}


bool Time::operator==(const Time& rhs) const {
  if (hour == rhs.GetHour() && minute == rhs.GetMinute()) {
    return true;
  }
  else {
    return false;
  }
}