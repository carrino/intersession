// Screen.ino: the readings on the board's own screen, laid out like the factory demo.
//
// What it adds: showBleStatus() draws the board's name on the top line and showNextLine()
// redraws one reading line per call.
//
// Why: text drawn straight to the screen goes out as hundreds of tiny SPI transfers and
// blocks the loop for tens of milliseconds a line, long enough to miss motion samples.
// Each line is drawn into lineCanvas in RAM instead and pushed as one block, which takes
// about two, and only one line is drawn per tick.

void setupDisplay() {
  display.init(240, 240);
  display.setRotation(1);
  display.fillScreen(ST77XX_BLACK);
  pinMode(TFT_BACKLIGHT_PIN, OUTPUT);
  digitalWrite(TFT_BACKLIGHT_PIN, HIGH);
  display.setTextSize(2);
  display.setCursor(0, 16);  // the top line is for the BLE name and state
}

// The board's name, yellow while waiting for a page or phone and green once one connects.
// A twelve-letter name leaves no room on the line for the words.
void showBleStatus() {
  startLine();
  lineCanvas.setTextColor(Bluefruit.connected() ? ST77XX_GREEN : ST77XX_YELLOW);
  lineCanvas.print(bleName);
  pushLine(0);
}

void showNextLine() {
  static int line = 0;
  startLine();
  switch (line) {
    case 0: showValue("Temp: ", readings.temperature, 2, " C"); break;
    case 1: showValue("Baro: ", readings.pressure, 2, " hPa"); break;
    case 2: showValue("Humid: ", readings.humidity, 2, " %"); break;
    case 3: showValue("Light: ", readings.color[3], 0, ""); break;
    case 4: showValue("Prox: ", readings.proximity, 0, ""); break;
    case 5: showVector("Accel:", readings.vectors.accel); break;
    case 6: showVector("Gyro:", readings.vectors.gyro); break;
    case 7: showVector("Mag:", readings.vectors.mag); break;
    case 8: showValue("Mic: ", readings.mic, 0, " dB"); break;
    case 9: showPlugins(); break;
    case 10: showTouch(); break;
  }
  pushLine(readingsTop + line * 16);
  line = (line + 1) % 11;
}

void startLine() {
  lineCanvas.fillScreen(ST77XX_BLACK);
  lineCanvas.setTextSize(2);
  lineCanvas.setTextColor(ST77XX_WHITE);
  lineCanvas.setCursor(0, 0);
}

void pushLine(int y) {
  display.drawRGBBitmap(0, y, lineCanvas.getBuffer(), lineCanvas.width(), lineCanvas.height());
}

void showValue(const char *label, float value, int decimals, const char *unit) {
  lineCanvas.print(label);
  lineCanvas.print(value, decimals);
  lineCanvas.print(unit);
}

void showVector(const char *label, const float v[3]) {
  lineCanvas.print(label);
  lineCanvas.print(v[0], 1);
  lineCanvas.print(",");
  lineCanvas.print(v[1], 1);
  lineCanvas.print(",");
  lineCanvas.print(v[2], 1);
}
