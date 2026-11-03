// Day 1: the CLUE factory demo, plus an orientation fused from the motion
// sensors and every reading published over Bluetooth LE for orientation.html.
//
// Needs the Adafruit nRF52 board package and the libraries listed in README.md.
// The Quaternion and MagCalibration files sit next to this sketch and compile with it.

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
const int QUATERNION_HZ = 30;  // the orientation, the one thing the page needs fast
const int DEBUG_HZ = 10;       // accel, gyro, mag, color, proximity, mic
const int SLOW_HZ = 2;         // temperature, pressure, humidity, mag fit, stats
const int SCREEN_LINES_HZ = 20;  // one line of the screen per tick: 9 lines refresh every 0.45 s

// How far toward the accelerometer's idea of "down" we move on each IMU sample, and how
// far toward the magnetometer's idea of "north" on each mag reading. Small is smooth but
// slow to fix gyro drift; large is quick but jittery.
const float ACCEL_CORRECTION = 0.005;  // 416 times a second
const float MAG_CORRECTION = 0.05;     // 10 times a second

// The PDM driver's gain register is 0.5 dB per step with 40 as 0 dB. The default of 20
// (-10 dB) leaves a quiet room below the converter's smallest step, so everything quiet
// reads as the same hiss; +10 dB lifts it clear while a loud clap still fits.
const int MIC_GAIN = 60;

// Turns the mic level relative to full scale into an approximate sound pressure level:
// the mic puts out -26 dBFS at 94 dB SPL, and the +10 dB of gain comes off again.
const float MIC_DB_OFFSET = 94 + 26 - 10;

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

// One-time per-axis offsets for this board's sensors, found by the page's calibration
// screen (hold each face of the board toward the floor, still, for a second) and kept in
// the board's flash. Each reading becomes raw - offset. An unwritten board has all zeros,
// which means "not calibrated": then the readings are used raw, the sphere fit and the
// north correction stay off, and the page offers the calibration screen.
struct Calibration {
  float accelOffset[3];
  float gyroOffset[3];
  float magOffset[3];
};
Calibration calibration;
const char *CALIBRATION_FILE = "/calibration.bin";

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
  float stats[3];       // IMU samples fused per second; degrees between where we think down is and true
                        // down; updates subscribers missed because the radio was busy
  float temperature;    // C
  float pressure;       // hPa
  float humidity;       // percent
  float color[4];       // red, green, blue, clear light counts
  float proximity;      // 0 far to 255 near
  float mic;            // approximate dB SPL
  uint8_t buttons;      // bit 0 = A, bit 1 = B
};
Readings readings;
char logLine[64];

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
  {logChar, logLine, sizeof(logLine)},
};

// What loop() is doing, so a stall can be reported. The names match the enum order.
enum Stage { STAGE_NONE, STAGE_BLE, STAGE_IMU, STAGE_MAG, STAGE_LIGHT_MIC, STAGE_ENVIRONMENT, STAGE_SCREEN, STAGE_COUNT };
const char *STAGE_NAMES[STAGE_COUNT] = {"nothing", "BLE", "the IMU", "the magnetometer", "the light sensor or mic",
                                        "the environment sensors", "the screen"};
volatile Stage stage = STAGE_NONE;
volatile uint32_t loopAliveMs = 0;  // when loop() last checked in
SoftwareTimer stallTimer;

char bleName[16];     // CLUE-xxxx, shown with the connection state on the top line
bool bleStarted = false;
int readingsTop = 0;  // screen row where the live readings start, below any warnings
int imuSamples = 0;   // fused since the last stats update

Adafruit_ST7789 display(&SPI1, TFT_CS_PIN, TFT_DC_PIN, TFT_RESET_PIN);
GFXcanvas16 lineCanvas(240, 16);  // one line of size-2 text, drawn in RAM then pushed as a block
Adafruit_NeoPixel pixel(1, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

// CLUE boards ship with one of two nearly identical motion chips.
Adafruit_LSM6DS33 lsm6ds33;
Adafruit_LSM6DS3TRC lsm6ds3trc;
Adafruit_LSM6DS *imu = NULL;
Adafruit_LIS3MDL lis3mdl;
Adafruit_APDS9960 apds9960;
Adafruit_BMP280 bmp280;
MagCalibration magCalibration;

// The rotation that takes a vector in the device's frame to the world frame:
//   world = orientation.rotate(device)
// The device's frame is the motion chip's: x toward button A, y toward the USB plug,
// z down through the screen. The world frame is NED: x north, y east, z down. Identity
// is the board flat, screen up, with button A's edge toward north.
Quaternion orientation;

// Set by the IMU's data-ready interrupt, cleared when loop() takes the sample.
// The CPU cycle counter gives a precise timestamp for when the sample was taken.
volatile bool sampleReady = false;
volatile uint32_t sampleCycles = 0;

// Running sums of the microphone samples since last checked, kept by the PDM callback.
volatile int64_t micSum = 0, micSumSquares = 0;
volatile int32_t micCount = 0;
int16_t micSamples[256];

void setup() {
  Serial.begin(115200);
  dwt_enable();  // start the CPU cycle counter
  pinMode(WHITE_LED_PIN, OUTPUT);
  pinMode(BUTTON_A_PIN, INPUT_PULLUP);
  pinMode(BUTTON_B_PIN, INPUT_PULLUP);
  pixel.begin();

  setupDisplay();
  reportLastReset();
  loadCalibration();
  unstickI2cBus();
  setupSensors();
  setupMic();
  setupImu();
  setupBle();
  readingsTop = display.getCursorY() + 8;
  startWatchdog();
  stallTimer.begin(500, onStallCheck);
  stallTimer.start();
  note(calibrated() ? "Started, calibrated" : "Started, not calibrated: open the page to calibrate");
}

void loop() {
  static uint32_t lastQuaternionMs = 0, lastDebugMs = 0, lastSlowMs = 0, lastScreenMs = 0;
  NRF_WDT->RR[0] = WDT_RR_RR_Reload;  // tell the watchdog we are still running
  loopAliveMs = millis();            // and the stall timer

  stage = STAGE_BLE;
  flushPublished();

  // Data-ready stays high until we read, so also go when it is already high: a sample
  // that landed before the interrupt was attached never produces a rising edge.
  stage = STAGE_IMU;
  if (sampleReady || digitalRead(IMU_INTERRUPT_PIN)) {
    readImu();
  }
  if (every(1000 / QUATERNION_HZ, lastQuaternionMs)) {
    publish(quaternionChar);
    handleButtons();
  }
  if (every(1000 / DEBUG_HZ, lastDebugMs)) {
    stage = STAGE_MAG;
    readMag();
    stage = STAGE_LIGHT_MIC;
    readLightAndMic();
    publish(vectorsChar);
    publish(colorChar);
    publish(proximityChar);
    publish(micChar);
  }
  if (every(1000 / SLOW_HZ, lastSlowMs)) {
    stage = STAGE_ENVIRONMENT;
    readEnvironment();
    fitMag();
    updateStats();
    publish(temperatureChar);
    publish(pressureChar);
    publish(humidityChar);
    publish(magFitChar);
    publish(statsChar);
    checkSensorsAnswer();
    noteLinkSettings();
    stage = STAGE_SCREEN;
    showBleStatus();
  }
  if (every(1000 / SCREEN_LINES_HZ, lastScreenMs)) {
    stage = STAGE_SCREEN;
    showNextLine();
    analogWrite(LED_BUILTIN, 255 - millis() % 256);  // the factory demo's red LED pulse
  }
}

// ---- Motion sensors and orientation ----

void onImuDataReady() {
  sampleCycles = DWT->CYCCNT;
  sampleReady = true;
}

void readImu() {
  static uint32_t lastSampleCycles = DWT->CYCCNT;
  sampleReady = false;
  imuSamples++;
  float dt = (float)(sampleCycles - lastSampleCycles) / SystemCoreClock;
  lastSampleCycles = sampleCycles;

  sensors_event_t accelEvent, gyroEvent, tempEvent;
  imu->getEvent(&accelEvent, &gyroEvent, &tempEvent);
  float *accel = accelEvent.acceleration.v;
  float *gyro = gyroEvent.gyro.v;
  if (calibrated()) {
    for (int i = 0; i < 3; i++) {
      accel[i] -= calibration.accelOffset[i];
      gyro[i] -= calibration.gyroOffset[i];
    }
  }

  updateOrientation(accel, gyro, dt);
  readings.quaternion[0] = orientation.a;
  readings.quaternion[1] = orientation.b;
  readings.quaternion[2] = orientation.c;
  readings.quaternion[3] = orientation.d;

  copyVector(readings.vectors.accel, accel);
  copyVector(readings.vectors.gyro, gyro);
  float accelNed[3];
  toWorldFrame(accelNed, accel);
  lowPass(readings.vectors.accelNed, accelNed, 0.05);  // about the last 50 ms at 416 Hz
}

// Multiplying the orientation on the right applies a rotation in the device's frame;
// multiplying on the left applies one in the world frame.
void updateOrientation(const float accel[3], const float gyro[3], float dt) {
  // The gyro measures how the device turned, in the device's frame, in radians per
  // second. Apply that turn in full.
  Quaternion turn = Quaternion::from_axis_angle(gyro[0] * dt, gyro[1] * dt, gyro[2] * dt);
  orientation *= turn;
  orientation.normalize();

  // The accelerometer feels the push holding the board up, so minus its reading is down
  // in the device's frame. Rotate that into the world, where down is really (0, 0, 1),
  // and nudge the orientation a fraction of the way from where we think down is to there.
  Quaternion measuredDown(-accel[0], -accel[1], -accel[2]);
  Quaternion down = orientation.rotate(measuredDown.normalize());
  Quaternion correction = down.rotation_between_vectors(Quaternion(0, 0, 1));
  orientation = correction.fractional(ACCEL_CORRECTION) * orientation;

  readings.stats[1] = acos(constrain(down.d, -1, 1)) * 180 / PI;
}

// The accelerometer cannot tell which way is north; the magnetometer can, once its offset
// is known. Rotate the reading into the world, keep only its horizontal part (Earth's
// field also points steeply into the ground), and nudge the orientation about the
// vertical axis a fraction of the way from where that points to north, (1, 0, 0).
void correctNorth(const float mag[3]) {
  Quaternion field = orientation.rotate(Quaternion(mag[0], mag[1], mag[2]));
  field.d = 0;
  if (field.norm() < 5) {
    return;  // almost vertical: no usable sense of north in this reading
  }
  field.normalize();
  Quaternion correction = field.rotation_between_vectors(Quaternion(1, 0, 0));
  orientation = correction.fractional(MAG_CORRECTION) * orientation;
}

void updateStats() {
  static bool imuSilent = false;
  readings.stats[0] = imuSamples * SLOW_HZ;
  if ((imuSamples == 0) != imuSilent) {
    imuSilent = imuSamples == 0;
    imuSilent ? warn("No IMU samples") : note("IMU samples back");
  }
  imuSamples = 0;
}

// An address-only I2C transfer, the thing an I2C scanner does: a live sensor acknowledges
// it, a dead one does not. Logs each sensor's first miss and its recovery.
void checkSensorsAnswer() {
  for (Sensor &sensor : sensors) {
    Wire.beginTransmission(sensor.address);
    bool answering = Wire.endTransmission() == 0;
    if (answering != sensor.answering) {
      sensor.answering = answering;
      char message[64];
      snprintf(message, sizeof(message), "%s %s", sensor.name, answering ? "back" : "not answering");
      answering ? note(message) : warn(message);
    }
  }
}

// Raw until the board is calibrated. After that the stored offset comes off, the sphere
// fit watches for anything the stored offset missed (its own offset should stay near
// zero), and the reading steers north.
void readMag() {
  sensors_event_t magEvent;
  lis3mdl.getEvent(&magEvent);
  float *mag = magEvent.magnetic.v;
  if (calibrated()) {
    for (int i = 0; i < 3; i++) {
      mag[i] -= calibration.magOffset[i];
    }
    magCalibration.add(mag, readings.vectors.accel);
  }
  float *corrected = readings.vectors.mag;
  for (int i = 0; i < 3; i++) {
    corrected[i] = mag[i] - magCalibration.offset[i];
  }
  float strength = sqrt(corrected[0] * corrected[0] + corrected[1] * corrected[1] + corrected[2] * corrected[2]);
  if (calibrated() && strength > 20 && strength < 70) {  // Earth's field, not a desk or a magnet
    correctNorth(corrected);
  }
  float magNed[3];
  toWorldFrame(magNed, readings.vectors.mag);
  lowPass(readings.vectors.magNed, magNed, 0.2);  // about the last half second at 10 Hz
}

// Refit the magnetometer's hard-iron offset. The fit only switches on once every face of
// the board has been pointed at the floor; until then the offset stays at zero.
void fitMag() {
  static bool locked = false;
  if (magCalibration.solve() && !locked) {
    locked = true;
    char message[64];
    snprintf(message, sizeof(message), "Mag fit locked: offset %.0f %.0f %.0f, field %.0f uT", magCalibration.offset[0],
             magCalibration.offset[1], magCalibration.offset[2], magCalibration.radius);
    note(message);
  }
  copyVector(readings.magFit, magCalibration.offset);
  readings.magFit[3] = magCalibration.radius;
  readings.magFit[4] = magCalibration.facesSeen();
}

// Rotates a vector from the device's frame into the world frame.
void toWorldFrame(float dest[3], const float v[3]) {
  Quaternion world = orientation.rotate(Quaternion(v[0], v[1], v[2]));
  dest[0] = world.b;
  dest[1] = world.c;
  dest[2] = world.d;
}

// Moves a smoothed vector the given fraction of the way toward the latest reading.
void lowPass(float smoothed[3], const float latest[3], float fraction) {
  for (int i = 0; i < 3; i++) {
    smoothed[i] += (latest[i] - smoothed[i]) * fraction;
  }
}

// ---- The other sensors ----

void readEnvironment() {
  readings.temperature = bmp280.readTemperature();
  readings.pressure = bmp280.readPressure() / 100;
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

// The RMS level of the microphone samples since the last call, in decibels.
float micDecibels() {
  noInterrupts();  // so the PDM callback cannot change the sums while we take them
  int64_t sum = micSum, sumSquares = micSumSquares;
  int32_t count = micCount;
  micSum = micSumSquares = micCount = 0;
  interrupts();
  if (count == 0) {
    return 0;
  }
  float mean = (float)sum / count;
  float rms = sqrt(max((float)sumSquares / count - mean * mean, 1.0f));  // 1 keeps log10 finite in silence
  return 20 * log10(rms / 32768) + MIC_DB_OFFSET;
}

// Called by the PDM library whenever a chunk of microphone samples is ready.
void onMicData() {
  int count = PDM.read(micSamples, sizeof(micSamples)) / sizeof(micSamples[0]);
  for (int i = 0; i < count; i++) {
    micSum += micSamples[i];
    micSumSquares += (int32_t)micSamples[i] * micSamples[i];
  }
  micCount += count;
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
// characteristic. The fixed length means any other size was already rejected.
void onPixelWrite(uint16_t connHandle, BLECharacteristic *chr, uint8_t *rgb, uint16_t len) {
  pixel.fill(pixel.Color(rgb[0], rgb[1], rgb[2]));
  pixel.show();
}

// ---- Calibration, kept in the board's flash ----

bool calibrated() {
  static const Calibration none = {};
  return memcmp(&calibration, &none, sizeof(calibration)) != 0;
}

void loadCalibration() {
  InternalFS.begin();
  Adafruit_LittleFS_Namespace::File file(InternalFS);
  if (file.open(CALIBRATION_FILE, Adafruit_LittleFS_Namespace::FILE_O_READ)) {
    file.read(&calibration, sizeof(calibration));
    file.close();
  }
}

void saveCalibration() {
  InternalFS.remove(CALIBRATION_FILE);  // FILE_O_WRITE appends, so start clean
  Adafruit_LittleFS_Namespace::File file(InternalFS);
  if (file.open(CALIBRATION_FILE, Adafruit_LittleFS_Namespace::FILE_O_WRITE)) {
    file.write((const uint8_t *)&calibration, sizeof(calibration));
    file.close();
  }
}

// Runs when the page writes a Calibration. All zeros means "forget it and start over".
void onCalibrationWrite(uint16_t connHandle, BLECharacteristic *chr, uint8_t *data, uint16_t len) {
  memcpy(&calibration, data, sizeof(calibration));
  if (calibrated()) {
    saveCalibration();
    note("Calibration saved");
  } else {
    InternalFS.remove(CALIBRATION_FILE);
    note("Calibration cleared");
  }
}

// ---- Setup ----

void setupDisplay() {
  display.init(240, 240);
  display.setRotation(1);
  display.fillScreen(ST77XX_BLACK);
  pinMode(TFT_BACKLIGHT_PIN, OUTPUT);
  digitalWrite(TFT_BACKLIGHT_PIN, HIGH);
  display.setTextSize(2);
  display.setCursor(0, 16);  // the top line is for the BLE name and state
}

// Runs in its own task twice a second. If loop() has not checked in for a second it is
// stuck, almost certainly in an I2C transfer that will never finish. The radio is still
// running, so over the next three ticks: say what it was doing, on the screen and over
// BLE, and keep that in a register that survives the reset; hang up, so the page hears
// at once instead of waiting out its link timeout; then reset the chip.
void onStallCheck(TimerHandle_t timer) {
  static int ticksStuck = 0;
  if (millis() - loopAliveMs < 1000) {
    ticksStuck = 0;
    return;
  }
  ticksStuck++;
  if (ticksStuck == 1) {
    char message[64];
    snprintf(message, sizeof(message), "Stuck in %s, rebooting", STAGE_NAMES[stage]);
    warn(message);
    notify(find(logChar));  // the stuck loop will not flush it, so send it from here
    sd_power_gpregret_clr(1, 0xFF);
    sd_power_gpregret_set(1, stage);
  } else if (ticksStuck == 2) {
    Bluefruit.disconnect(Bluefruit.connHandle());
  } else {
    NVIC_SystemReset();
  }
}

// After a reset, say why if it was ours. Runs before the radio starts, while the retained
// register can still be read directly; the stall check left the stuck stage in it.
void reportLastReset() {
  uint8_t stuckIn = NRF_POWER->GPREGRET2;
  NRF_POWER->GPREGRET2 = 0;
  if (stuckIn != 0) {
    char message[64];
    snprintf(message, sizeof(message), "Rebooted: was stuck in %s", STAGE_NAMES[stuckIn % STAGE_COUNT]);
    warn(message);
  } else if (readResetReason() & POWER_RESETREAS_DOG_Msk) {
    warn("Rebooted by the watchdog");
  }
}

// The backstop behind the stall check: a hardware timer that resets the chip if loop()
// stops reporting in for five seconds, even if the stall check itself is wedged. The bus
// is cleared on the way back up. A delay() longer than five seconds trips it.
void startWatchdog() {
  NRF_WDT->CONFIG = WDT_CONFIG_SLEEP_Run << WDT_CONFIG_SLEEP_Pos;  // keeps counting while asleep
  NRF_WDT->CRV = 5 * 32768;                                        // five seconds of its 32768 Hz clock
  NRF_WDT->RREN = WDT_RREN_RR0_Msk;
  NRF_WDT->TASKS_START = 1;
}

// A reset in the middle of an I2C read, which every upload risks, leaves that sensor
// holding the data line low until it is clocked out, and every later I2C transfer would
// then wait forever. Pulse the clock by hand until the data line is released, then send
// a stop. Wire takes the pins over afterwards.
void unstickI2cBus() {
  pinMode(PIN_WIRE_SDA, INPUT_PULLUP);
  pinMode(PIN_WIRE_SCL, OUTPUT_S0D1);  // open drain, like an I2C master
  digitalWrite(PIN_WIRE_SCL, HIGH);
  for (int i = 0; i < 9 && digitalRead(PIN_WIRE_SDA) == LOW; i++) {
    digitalWrite(PIN_WIRE_SCL, LOW);
    delayMicroseconds(5);
    digitalWrite(PIN_WIRE_SCL, HIGH);
    delayMicroseconds(5);
  }
  pinMode(PIN_WIRE_SDA, OUTPUT_S0D1);
  digitalWrite(PIN_WIRE_SDA, LOW);  // stop condition: data rises while the clock is high
  delayMicroseconds(5);
  digitalWrite(PIN_WIRE_SDA, HIGH);
}

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
  if (lsm6ds33.begin_I2C()) {
    imu = &lsm6ds33;
  } else if (lsm6ds3trc.begin_I2C()) {
    imu = &lsm6ds3trc;
  }
  if (!lis3mdl.begin_I2C()) {
    warn("LIS3MDL missing");
  }
  Wire.beginTransmission(SHT30_ADDRESS);
  Wire.write(0x21);  // periodic mode: measure once a second, high repeatability
  Wire.write(0x30);
  if (Wire.endTransmission() != 0) {
    warn("SHT30 missing");
  }
  if (!bmp280.begin()) {
    warn("BMP280 missing");
  }
  Wire.setClock(400000);  // every chip supports fast mode I2C; their begin() calls left it at 100 kHz
}

void setupMic() {
  PDM.onReceive(onMicData);
  if (!PDM.begin(1, 16000)) {
    warn("Mic failed to start");
  }
  PDM.setGain(MIC_GAIN);
}

// Sample at 416 Hz and raise INT1 whenever a new gyro reading is ready.
void setupImu() {
  if (imu == NULL) {
    halt("No IMU found");
  }
  imu->setGyroDataRate(LSM6DS_RATE_416_HZ);
  imu->setAccelDataRate(LSM6DS_RATE_416_HZ);
  imu->configInt1(false, true, false);  // (temperature, gyro, accel) data-ready on INT1
  pinMode(IMU_INTERRUPT_PIN, INPUT);
  attachInterrupt(IMU_INTERRUPT_PIN, onImuDataReady, RISING);
}

void setupBle() {
  // Max MTU 247 so the 60-byte vectors packet fits, and room to queue 8 notifications.
  Bluefruit.configPrphConn(247, BLE_GAP_EVENT_LENGTH_DEFAULT, 8, 1);
  Bluefruit.begin();

  // Give each CLUE its own name so a room full of them is easy to tell apart.
  ble_gap_addr_t addr = Bluefruit.getAddr();
  snprintf(bleName, sizeof(bleName), "CLUE-%02X%02X", addr.addr[1], addr.addr[0]);
  Bluefruit.setName(bleName);
  showBleStatus();

  sensorService.begin();  // characteristics join the most recently begun service
  for (Published &p : published) {
    beginNotifyChar(p.chr, p.len);
  }

  pixelChar.setProperties(CHR_PROPS_READ | CHR_PROPS_WRITE | CHR_PROPS_WRITE_WO_RESP);
  pixelChar.setFixedLen(3);
  pixelChar.setWriteCallback(onPixelWrite);
  pixelChar.begin();

  calibrationChar.setProperties(CHR_PROPS_READ | CHR_PROPS_WRITE);
  calibrationChar.setFixedLen(sizeof(calibration));
  calibrationChar.setWriteCallback(onCalibrationWrite);
  calibrationChar.begin();
  calibrationChar.write(&calibration, sizeof(calibration));

  // Our 128-bit UUID nearly fills the advertising packet, so the name goes in the scan
  // response, which scanners ask for right after.
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addService(sensorService);
  Bluefruit.ScanResponse.addName();
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 32);  // every 20 ms, always, so reconnecting is quick
  Bluefruit.Advertising.start(0);             // 0 = keep advertising until connected
  bleStarted = true;
}

// Reports the radio settings the laptop or phone agreed to, once they have settled and
// whenever they change: the MTU decides whether the bigger packets arrive whole, the
// interval is the ceiling on how many packets a second get through, and the timeout is
// how long the other side waits in silence before it declares the link dead.
void noteLinkSettings() {
  static uint16_t lastMtu = 0, lastInterval = 0, lastTimeout = 0;
  BLEConnection *conn = Bluefruit.Connection(Bluefruit.connHandle());
  if (!Bluefruit.connected() || conn == NULL) {
    lastMtu = 0;
    return;
  }
  if (conn->getMtu() != lastMtu || conn->getConnectionInterval() != lastInterval ||
      conn->getSupervisionTimeout() != lastTimeout) {
    lastMtu = conn->getMtu();
    lastInterval = conn->getConnectionInterval();
    lastTimeout = conn->getSupervisionTimeout();
    char message[64];
    snprintf(message, sizeof(message), "Link: MTU %u, interval %.2f ms, timeout %u ms, %u Mbit PHY", lastMtu,
             lastInterval * 1.25, lastTimeout * 10, conn->getPHY() == BLE_GAP_PHY_2MBPS ? 2 : 1);
    note(message);
  }
}

// A characteristic anyone can read or subscribe to, holding a fixed number of bytes.
void beginNotifyChar(BLECharacteristic &chr, int bytes) {
  chr.setProperties(CHR_PROPS_READ | CHR_PROPS_NOTIFY);
  chr.setFixedLen(bytes);
  chr.begin();
}

Published &find(BLECharacteristic &chr) {
  for (Published &p : published) {
    if (&p.chr == &chr) {
      return p;
    }
  }
  halt("publish() of a characteristic that is not in the published table");
  return published[0];
}

// Stores a characteristic's current value and marks it changed. If the previous change
// never reached subscribers, they will skip straight to this one: count that.
void publish(BLECharacteristic &chr) {
  Published &p = find(chr);
  if (p.changed) {
    readings.stats[2]++;
  }
  p.chr.write(p.value, p.len);
  p.changed = true;
}

// Sends changed values to subscribers, stopping when the radio queue is full and picking
// up where it left off on the next loop pass.
void flushPublished() {
  for (Published &p : published) {
    if (!p.changed) {
      continue;
    }
    if (!p.chr.notifyEnabled()) {
      p.changed = false;  // nobody is listening; the value is still there to be read
      continue;
    }
    if (notify(p) == NRF_ERROR_RESOURCES) {
      return;
    }
    p.changed = false;
  }
}

// Hands one value to the radio. Bluefruit's own notify() waits up to 100 ms when the
// queue is full, which would stall the loop; this returns at once either way.
uint32_t notify(Published &p) {
  ble_gatts_hvx_params_t hvx = {};
  uint16_t len = p.len;
  hvx.handle = p.chr.handles().value_handle;
  hvx.type = BLE_GATT_HVX_NOTIFICATION;
  hvx.p_len = &len;
  hvx.p_data = (uint8_t *)p.value;
  return sd_ble_gatts_hvx(Bluefruit.connHandle(), &hvx);
}

// ---- Screen, laid out like the factory demo ----

// Text drawn straight to the screen goes out as hundreds of tiny SPI transfers and blocks
// the loop for tens of milliseconds a line. Each line is drawn into lineCanvas instead
// and pushed as one block, which takes about two, and only one line is drawn per tick.

void showBleStatus() {
  startLine();
  lineCanvas.print(bleName);
  lineCanvas.print(Bluefruit.connected() ? " connected" : " waiting");
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
  }
  pushLine(readingsTop + line * 16);
  line = (line + 1) % 9;
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
