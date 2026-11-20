// Touch.ino: the three ring pads P0, P1 and P2 as touch pads.
//
// What it adds: readTouch() fills readings.touch with a count per pad that rises while a
// finger, or anything conductive clipped to the pad, is on it. showTouch() is its line on
// the screen.
//
// Why: the CLUE has no touch chip, and Arduino for this board has no touchRead(). This is
// the trick CircuitPython's touchio uses: charge a pad, let it float, and count how long it
// stays high. A finger adds capacitance and a leak to ground, so the count changes.

const int TOUCH_PINS[3] = {0, 1, 2};  // the P0, P1, P2 ring pads on the bottom edge

// The count stops at TOUCH_MAX_TICKS so a pad that never falls (nothing to discharge it; a
// 1 megohm resistor from the pad to GND fixes that) cannot stall the loop.
const int TOUCH_SAMPLES = 4;
const int TOUCH_MAX_TICKS = 2000;

void readTouch() {
  for (int i = 0; i < 3; i++) {
    readings.touch[i] = touchRead(TOUCH_PINS[i]);
  }
}

// How long, in loop ticks, the pad stays high after being charged and let go.
int touchRead(int pin) {
  int ticks = 0;
  for (int sample = 0; sample < TOUCH_SAMPLES; sample++) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, HIGH);
    delayMicroseconds(10);
    pinMode(pin, INPUT);
    while (digitalRead(pin) == HIGH && ticks < TOUCH_MAX_TICKS) {
      ticks++;
    }
  }
  return ticks;
}

void showTouch() {
  lineCanvas.print("Touch: ");
  for (int i = 0; i < 3; i++) {
    lineCanvas.print(i ? " " : "");
    lineCanvas.print((int)readings.touch[i]);
  }
}
