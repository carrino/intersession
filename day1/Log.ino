// Log.ino: what the board has to say.
//
// What it adds: note() sends a line to the serial port and to the log characteristic,
// which the page prints under its table. warn() is the same for a problem: also on the
// screen in red, and flagged as an error on the page. halt() is for a problem there is no
// carrying on from.
//
// Why: once the board is off the cable there is no serial monitor, and the screen is
// small. This is the way to see what the board is thinking from the page. The lines wait
// in a short queue until a page is listening, so what the board said while starting up,
// such as why it rebooted, is the first thing a page hears when it connects.

const int LOG_LINES = 8;  // how many lines can wait; older ones are dropped
char logQueue[LOG_LINES][sizeof(logLine)];
volatile int logSaid = 0;  // lines said so far
int logSent = 0;           // lines handed to the radio so far

void note(const char *message) {
  // Only if the serial port has room: a write to a port that is open but not being read
  // waits forever, and that would freeze whatever called this.
  if (Serial.availableForWrite() >= (int)strlen(message) + 2) {
    Serial.println(message);
  }
  strncpy(logQueue[logSaid % LOG_LINES], message, sizeof(logLine) - 1);
  logSaid++;
}

// Sends the next waiting line, once a page is listening and the line before has gone.
void sendLog() {
  if (logSaid - logSent > LOG_LINES) {
    logSent = logSaid - LOG_LINES;  // the oldest were written over before anyone listened
  }
  if (logSent == logSaid || !logChar.notifyEnabled() || find(logChar).changed) {
    return;
  }
  memcpy(logLine, logQueue[logSent % LOG_LINES], sizeof(logLine));
  logSent++;
  publish(logChar);
}

// Sends the newest line straight to the radio. For the stall check, which runs while
// loop() is stuck and sendLog() is not being called.
void sendNewestLineNow() {
  memcpy(logLine, logQueue[(logSaid - 1) % LOG_LINES], sizeof(logLine));
  notify(find(logChar));
}

// Like note(), for a problem: also on the screen in red, and flagged as an error on the page.
void warn(const char *message) {
  display.setTextColor(ST77XX_RED);
  display.println(message);
  char line[sizeof(logLine)];
  snprintf(line, sizeof(line), "ERROR: %s", message);
  note(line);
}

void halt(const char *message) {
  warn(message);
  while (true) {
  }
}
