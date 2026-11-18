// Day 1: the CLUE factory demo, plus an orientation fused from the motion
// sensors and every reading published over Bluetooth LE for orientation.html.
//
// Needs the Adafruit nRF52 board package and the libraries listed in README.md.
//
// This file is the whole picture: the pins, the list of readings, setup() and loop(), and
// the sensors that take one library call to read. Anything that takes more than that has
// its own tab next to this one, with a header saying what it adds and why. Open the ones
// your project uses:
//
//   Orientation.ino   which way the board is facing, from the gyro, accelerometer and compass
//   Calibration.ino   this board's sensor offsets, kept in its flash
//   MotionEvents.ino  taps, double taps, falls and steps, spotted by the motion chip itself
//   Sound.ino         the microphone's loudness and its spectrum
//   Beat.ino          finds the beat in music and flashes the back NeoPixel on it
//   Touch.ino         the P0, P1 and P2 ring pads as touch pads
//   Plugins.ino       notices STEMMA QT boards being plugged in
//   Bluetooth.ino     how the readings get to the page
//   Name.ino          a name for each board
//   Storage.ino       small files in the chip's flash, saved so a power cut cannot spoil them
//   Screen.ino        the readings on the board's own screen
//   Log.ino           what the board has to say: note() and warn()
//   Watchdog.ino      gets the board going again by itself when the loop stops
//
// The tabs are all one program: the Arduino IDE joins them to the end of this file, so
// they share everything declared here. Each keeps its own constants and variables at its
// top, and the rest of the program uses it only through its functions.

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_LSM6DS33.h>
#include <Adafruit_LSM6DS3TRC.h>
#include <Adafruit_LIS3MDL.h>
#include <Adafruit_APDS9960.h>
#include <Adafruit_BMP280.h>
#include <PDM.h>
#include <Wire.h>
#include <bluefruit.h>
#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>
#include <arm_math.h>  // ARM's DSP library, for the FFT; the board package links it in
#include "Quaternion.h"
#include "MagCalibration.h"

// CLUE pin numbers.
const int TFT_CS_PIN = 31, TFT_DC_PIN = 32, TFT_RESET_PIN = 33, TFT_BACKLIGHT_PIN = 34;
const int BUTTON_A_PIN = 5, BUTTON_B_PIN = 11;  // pressed = LOW
const int NEOPIXEL_PIN = 18;
const int SPEAKER_PIN = 46;
const int WHITE_LED_PIN = 43;
const int IMU_INTERRUPT_PIN = 45;  // the IMU's INT1 line

// Every sensor is on the one I2C bus. The addresses are checked twice a second to catch
// a sensor that has stopped answering, since the sensor libraries do not report that.
struct Sensor {
  const char *name;
  uint8_t address;
  bool answering;
};
Sensor sensors[] = {
  {"APDS9960", 0x39, true}, {"LSM6DS", 0x6A, true}, {"LIS3MDL", 0x1C, true}, {"SHT30", 0x44, true}, {"BMP280", 0x77, true},
};
const int SHT30_ADDRESS = 0x44;

// How often each group of readings is sent, in packets per second.
const int QUATERNION_HZ = 30;  // the orientation, plus checking the buttons, taps and falls
const int DEBUG_HZ = 10;       // accel, gyro, mag, color, proximity, mic, touch, pressure
const int SLOW_HZ = 2;         // temperature, humidity, mag fit, stats
const int SCREEN_LINES_HZ = 20;  // one line of the screen per tick: 11 lines refresh every 0.55 s

const int SPECTRUM_BANDS = 13;  // the bands Sound.ino splits the mic's sound into

// Our BLE service, one characteristic per reading. Values are little-endian floats,
// except buttons and pixel which are bytes. orientation.html lists the same UUIDs.
BLEService sensorService("c10e0001-5a7e-4a6b-9f3e-0b6f2d9a1c01");
BLECharacteristic quaternionChar("c10e0002-5a7e-4a6b-9f3e-0b6f2d9a1c01");
BLECharacteristic vectorsChar("c10e0003-5a7e-4a6b-9f3e-0b6f2d9a1c01");
BLECharacteristic temperatureChar("c10e0006-5a7e-4a6b-9f3e-0b6f2d9a1c01");
BLECharacteristic pressureChar("c10e0007-5a7e-4a6b-9f3e-0b6f2d9a1c01");
BLECharacteristic humidityChar("c10e0008-5a7e-4a6b-9f3e-0b6f2d9a1c01");
BLECharacteristic colorChar("c10e0009-5a7e-4a6b-9f3e-0b6f2d9a1c01");
BLECharacteristic micChar("c10e000a-5a7e-4a6b-9f3e-0b6f2d9a1c01");
BLECharacteristic buttonsChar("c10e000b-5a7e-4a6b-9f3e-0b6f2d9a1c01");
BLECharacteristic magFitChar("c10e000c-5a7e-4a6b-9f3e-0b6f2d9a1c01");
BLECharacteristic pixelChar("c10e000d-5a7e-4a6b-9f3e-0b6f2d9a1c01");  // writable: red, green, blue
BLECharacteristic proximityChar("c10e000e-5a7e-4a6b-9f3e-0b6f2d9a1c01");
BLECharacteristic statsChar("c10e000f-5a7e-4a6b-9f3e-0b6f2d9a1c01");
BLECharacteristic logChar("c10e0010-5a7e-4a6b-9f3e-0b6f2d9a1c01");  // text: every note() and warn()
BLECharacteristic calibrationChar("c10e0011-5a7e-4a6b-9f3e-0b6f2d9a1c01");  // read/write: the Calibration below
BLECharacteristic pluginsChar("c10e0012-5a7e-4a6b-9f3e-0b6f2d9a1c01");
BLECharacteristic touchChar("c10e0013-5a7e-4a6b-9f3e-0b6f2d9a1c01");
BLECharacteristic nameChar("c10e0014-5a7e-4a6b-9f3e-0b6f2d9a1c01");  // read/write: the board's name, text
BLECharacteristic eventsChar("c10e0015-5a7e-4a6b-9f3e-0b6f2d9a1c01");
BLECharacteristic spectrumChar("c10e0016-5a7e-4a6b-9f3e-0b6f2d9a1c01");  // bytes: one per band
BLECharacteristic beatChar("c10e0017-5a7e-4a6b-9f3e-0b6f2d9a1c01");

// The motion readings, sent together in one packet. The NED pair are the raw readings
// rotated into the world frame by the orientation and averaged there, where gravity and
// Earth's field are constants: a check on the fusion. accelNed should sit at (0, 0, -9.8)
// and magNed should hold still as the board turns.
struct Vectors {
  float accel[3];     // m/s^2
  float gyro[3];      // rad/s
  float mag[3];       // microtesla, hard-iron offset removed
  float accelNed[3];  // m/s^2
  float magNed[3];    // microtesla
};

// The latest reading from every sensor. The screen and BLE both read from here.
struct Readings {
  float quaternion[4];  // w, x, y, z
  Vectors vectors;
  float magFit[5];      // offset x, y, z, field strength (microtesla), faces seen as bits +x -x +y -y +z -z
  float stats[4];       // IMU samples fused per second; degrees between where we think down is and true
                        // down; updates subscribers missed because the radio was busy; stall reboots
                        // since power-on
  float temperature;    // C
  float pressure;       // hPa
  float humidity;       // percent
  float color[4];       // red, green, blue, clear light counts
  float proximity;      // 0 far to 255 near
  float mic;            // approximate dB SPL
  float plugins;        // which plug-in sensors answer, a bit each in the order of plugins[]
  float touch[3];       // P0, P1, P2 pad counts, higher when touched
  float events[4];      // taps, double taps, falls and steps since power-on; sent only when
                        // one goes up, so the page notes when
  float beat[2];        // the music's tempo in beats per minute, 0 when there is no steady beat, and
                        // beats counted since power-on; sent on each beat
  uint8_t buttons;      // bit 0 = A, bit 1 = B
  uint8_t spectrum[SPECTRUM_BANDS];  // loudness of each band, 100 Hz to 8 kHz, 0 to 255
};
Readings readings;
char logLine[64];  // the log line on its way to the page; Log.ino has the ones waiting

// Our BLE "database". Each value lives in its characteristic, where anyone can read it.
// publish() marks a value changed and flushPublished() sends it to subscribers once the
// radio has room, so they always end up with the latest value. When values change faster
// than the radio can carry them, subscribers skip ahead and stats counts what they missed.
struct Published {
  BLECharacteristic &chr;
  const void *value;
  uint16_t len;
  bool changed;
};
Published published[] = {
  {quaternionChar, readings.quaternion, sizeof(readings.quaternion)},
  {vectorsChar, &readings.vectors, sizeof(readings.vectors)},
  {temperatureChar, &readings.temperature, sizeof(readings.temperature)},
  {pressureChar, &readings.pressure, sizeof(readings.pressure)},
  {humidityChar, &readings.humidity, sizeof(readings.humidity)},
  {colorChar, readings.color, sizeof(readings.color)},
  {proximityChar, &readings.proximity, sizeof(readings.proximity)},
  {micChar, &readings.mic, sizeof(readings.mic)},
  {buttonsChar, &readings.buttons, sizeof(readings.buttons)},
  {magFitChar, readings.magFit, sizeof(readings.magFit)},
  {statsChar, readings.stats, sizeof(readings.stats)},
  {pluginsChar, &readings.plugins, sizeof(readings.plugins)},
  {touchChar, readings.touch, sizeof(readings.touch)},
  {eventsChar, readings.events, sizeof(readings.events)},
  {spectrumChar, readings.spectrum, sizeof(readings.spectrum)},
  {beatChar, readings.beat, sizeof(readings.beat)},
  {logChar, logLine, sizeof(logLine)},
};

// What loop() is doing, so a stall can be reported. The names match the enum order.
enum Stage { STAGE_NONE, STAGE_BLE, STAGE_IMU, STAGE_MAG, STAGE_LIGHT_MIC, STAGE_ENVIRONMENT, STAGE_SCREEN, STAGE_COUNT };
const char *STAGE_NAMES[STAGE_COUNT] = {"nothing", "BLE", "the IMU", "the magnetometer", "the light sensor or mic",
                                        "the environment sensors", "the screen"};
volatile Stage stage = STAGE_NONE;

char bleName[24];     // CLUE-<name>, shown on the top line
int readingsTop = 0;  // screen row where the live readings start, below any warnings

Adafruit_ST7789 display(&SPI1, TFT_CS_PIN, TFT_DC_PIN, TFT_RESET_PIN);
GFXcanvas16 lineCanvas(240, 16);  // one line of size-2 text, drawn in RAM then pushed as a block
Adafruit_NeoPixel pixel(1, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
volatile uint32_t backColor = 0;  // the color the page's picker asked for; Beat.ino flashes over it
volatile bool backColorChanged = false;
Adafruit_APDS9960 apds9960;
Adafruit_BMP280 bmp280;

void setup() {
  Serial.begin(115200);
  dwt_enable();  // start the CPU cycle counter
  pinMode(WHITE_LED_PIN, OUTPUT);
  pinMode(BUTTON_A_PIN, INPUT_PULLUP);
  pinMode(BUTTON_B_PIN, INPUT_PULLUP);
  pixel.begin();

  setupDisplay();
  reportLastReset();
  setupStorage();
  loadCalibration();
  loadName();
  unstickI2cBus();
  setupSensors();
  setupMic();
  setupImu();
  // The sensor libraries leave the bus at 100 kHz, which makes the 14-byte IMU read take
  // most of the 2.4 ms between samples; 250 kHz makes it a quarter. Not 400 kHz: the bus
  // also runs to edge pads 19 and 20, the small fingers between the 3V and GND rings, and
  // a finger resting there adds enough capacitance to corrupt bits at that speed, which
  // leaves a sensor holding the bus and the board rebooting. (The nRF52840's own 400 kHz
  // timing is also out of spec, erratum 219.)
  Wire.setClock(250000);
  setupBle();
  readingsTop = display.getCursorY() + 8;
  startWatchdog();
  startStallCheck();
  char message[64];
  snprintf(message, sizeof(message), "Started with the %s, %s", imuName(),
           calibrated() ? "calibrated" : "not calibrated: open the page");
  note(message);
}

void loop() {
  static uint32_t lastQuaternionMs = 0, lastDebugMs = 0, lastSlowMs = 0, lastScreenMs = 0;
  stillRunning();

  stage = STAGE_BLE;
  sendLog();
  flushPublished();

  stage = STAGE_IMU;
  readImu();
  stage = STAGE_LIGHT_MIC;
  updateSpectrum();
  if (every(1000 / QUATERNION_HZ, lastQuaternionMs)) {
    publish(quaternionChar);
    handleButtons();
    stage = STAGE_IMU;
    if (readMotionEvents()) {
      publish(eventsChar);  // like the buttons: only when something happened
    }
    if (spectrumChar.notifyEnabled()) {
      sendSpectrum();
    }
    if (backColorChanged) {
      backColorChanged = false;
      showBackColor();
    }
  }
  if (every(1000 / DEBUG_HZ, lastDebugMs)) {
    stage = STAGE_MAG;
    readMag();
    stage = STAGE_LIGHT_MIC;
    readLightAndMic();
    readTouch();
    stage = STAGE_ENVIRONMENT;
    readings.pressure = bmp280.readPressure() / 100;  // often, so altitude follows the board up and down
    publish(vectorsChar);
    publish(colorChar);
    publish(proximityChar);
    publish(micChar);
    publish(touchChar);
    publish(pressureChar);
  }
  if (every(1000 / SLOW_HZ, lastSlowMs)) {
    stage = STAGE_ENVIRONMENT;
    readEnvironment();
    fitMag();
    updateImuStats();
    publish(temperatureChar);
    publish(humidityChar);
    publish(magFitChar);
    publish(statsChar);
    checkSensorsAnswer();
    checkPlugins();
    noteLinkSettings();
    stage = STAGE_SCREEN;
    showBleStatus();
  }
  if (every(1000 / SCREEN_LINES_HZ, lastScreenMs)) {
    stage = STAGE_SCREEN;
    showNextLine();
  }
}

// ---- The sensors that take one call to read ----

// The sensors are all on the board, so only a problem gets mentioned on the screen.
void setupSensors() {
  // Color: 50 ms of light gathering per reading, five times the default, so an object lit
  // by the front LEDs gives counts in the hundreds rather than single digits.
  if (apds9960.begin(50)) {
    apds9960.enableColor(true);
    apds9960.enableProximity(true);
  } else {
    warn("APDS9960 missing");
  }
  Wire.beginTransmission(SHT30_ADDRESS);
  Wire.write(0x21);  // periodic mode: measure once a second, high repeatability
  Wire.write(0x30);
  if (Wire.endTransmission() != 0) {
    warn("SHT30 missing");
  }
  // Pressure: the datasheet's indoor-navigation oversampling (about 26 readings a second)
  // with a light filter, so it is steady to a few centimeters and follows a lift in about
  // 0.2 s. The page turns it into feet above sea level.
  if (bmp280.begin()) {
    bmp280.setSampling(Adafruit_BMP280::MODE_NORMAL, Adafruit_BMP280::SAMPLING_X2,
                       Adafruit_BMP280::SAMPLING_X16, Adafruit_BMP280::FILTER_X4,
                       Adafruit_BMP280::STANDBY_MS_1);
  } else {
    warn("BMP280 missing");
  }
}

void readEnvironment() {
  readings.temperature = bmp280.readTemperature();
  readHumidity();
}

// The SHT30 humidity sensor is told at startup to measure on its own once a second, so
// a read is just fetching its latest result. (The Adafruit library instead starts a
// measurement and sleeps 20 ms for it, which would stall the loop.)
void readHumidity() {
  Wire.beginTransmission(SHT30_ADDRESS);
  Wire.write(0xE0);  // fetch the latest measurement
  Wire.write(0x00);
  if (Wire.endTransmission() != 0 || Wire.requestFrom(SHT30_ADDRESS, 6) != 6) {
    return;  // the first result is not ready yet; keep the old value
  }
  uint8_t data[6];  // temperature, its checksum, humidity, its checksum
  for (int i = 0; i < 6; i++) {
    data[i] = Wire.read();
  }
  readings.humidity = 100.0 * ((data[3] << 8) | data[4]) / 65535;
}

void readLightAndMic() {
  if (apds9960.colorDataReady()) {
    uint16_t r, g, b, c;
    apds9960.getColorData(&r, &g, &b, &c);
    readings.color[0] = r;
    readings.color[1] = g;
    readings.color[2] = b;
    readings.color[3] = c;
  }
  readings.proximity = apds9960.readProximity();
  readings.mic = micDecibels();
}

// Same as the factory demo: A beeps, B lights the white LEDs on the front. The color
// sensor reads reflected light, so hold B to light whatever is in front of it.
void handleButtons() {
  uint8_t buttons = (digitalRead(BUTTON_A_PIN) == LOW ? 1 : 0) | (digitalRead(BUTTON_B_PIN) == LOW ? 2 : 0);
  if (buttons != readings.buttons) {
    readings.buttons = buttons;
    publish(buttonsChar);
  }
  if (buttons & 1) {
    tone(SPEAKER_PIN, 4000, 100);
  } else {
    tone(SPEAKER_PIN, 0);
  }
  digitalWrite(WHITE_LED_PIN, (buttons & 2) ? HIGH : LOW);
}

// Runs when a phone or web page writes three bytes, red green blue, to the pixel
// characteristic. The fixed length means any other size was already rejected. This runs in
// the Bluetooth library's task, so it only notes the color; loop() lights the pixel.
void onPixelWrite(uint16_t connHandle, BLECharacteristic *chr, uint8_t *rgb, uint16_t len) {
  backColor = pixel.Color(rgb[0], rgb[1], rgb[2]);
  backColorChanged = true;
}

void showBackColor() {
  pixel.fill(backColor);
  pixel.show();
}

// An address-only I2C transfer, the thing an I2C scanner does: a live sensor acknowledges
// it, a dead one does not. Logs each sensor's first miss and its recovery.
void checkSensorsAnswer() {
  char message[64];
  for (Sensor &sensor : sensors) {
    bool answering = answers(sensor.address);
    if (answering != sensor.answering) {
      sensor.answering = answering;
      snprintf(message, sizeof(message), "%s %s", sensor.name, answering ? "back" : "not answering");
      answering ? note(message) : warn(message);
    }
  }
}

// Whether a chip at this address acknowledges its name being called on the I2C bus.
bool answers(uint8_t address) {
  Wire.beginTransmission(address);
  return Wire.endTransmission() == 0;
}

// ---- Small helpers ----

// True once every periodMs milliseconds. lastMs remembers when it was last true.
bool every(uint32_t periodMs, uint32_t &lastMs) {
  uint32_t now = millis();
  if (now - lastMs < periodMs) {
    return false;
  }
  lastMs = now;
  return true;
}

void copyVector(float dest[3], const float v[3]) {
  dest[0] = v[0];
  dest[1] = v[1];
  dest[2] = v[2];
}
