// Log.ino: what the board has to say.
//
// What it adds: note() sends a line to the serial port and to the log characteristic,
// which the page prints under its table. warn() is the same for a problem: also on the
// screen in red, and flagged as an error on the page. halt() is for a problem there is no
// carrying on from.
//
// Why: once the board is off the cable there is no serial monitor, and the screen is
// small. This is the way to see what the board is thinking from the page.

// Sends a line to the serial port and to the log characteristic, which the web page prints
// in its console: the way to see what the board is thinking once it is off the cable. A
// line from before BLE is up goes out once it starts, so a page connecting later gets it.
void note(const char *message) {
  Serial.println(message);
  strncpy(logLine, message, sizeof(logLine) - 1);
  if (bleStarted) {
    publish(logChar);
  }
}

// Like note(), for a problem: also on the screen in red, and flagged as an error on the page.
void warn(const char *message) {
  display.setTextColor(ST77XX_RED);
  display.println(message);
  char line[64];
  snprintf(line, sizeof(line), "ERROR: %s", message);
  note(line);
}

void halt(const char *message) {
  warn(message);
  while (true) {
  }
}
