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
#include <Adafruit_SHT31.h>
#include <Adafruit_APDS9960.h>
#include <Adafruit_BMP280.h>
#include <PDM.h>
#include <bluefruit.h>
#include "Quaternion.h"
#include "MagCalibration.h"

// CLUE pin numbers.
const int TFT_CS_PIN = 31, TFT_DC_PIN = 32, TFT_RESET_PIN = 33, TFT_BACKLIGHT_PIN = 34;
const int BUTTON_A_PIN = 5, BUTTON_B_PIN = 11;  // pressed = LOW
const int NEOPIXEL_PIN = 18;
const int SPEAKER_PIN = 46;
const int WHITE_LED_PIN = 43;
const int IMU_INTERRUPT_PIN = 45;  // the IMU's INT1 line

// How often each group of readings is sent, in packets per second.
const int QUATERNION_HZ = 30;  // the orientation, the one thing the page needs fast
const int DEBUG_HZ = 10;       // accel, gyro, mag, color, proximity, mic
const int SLOW_HZ = 2;         // temperature, pressure, humidity, mag fit, stats
const int SCREEN_LINES_HZ = 20;  // one line of the screen per tick: 9 lines refresh every 0.45 s

// How far toward the accelerometer's idea of "down" we move on each IMU sample.
// Small is smooth but slow to fix gyro drift; large is quick but jittery.
const float ACCEL_CORRECTION = 0.005;

// Turns the mic level relative to full scale into an approximate sound pressure level:
// the mic puts out -26 dBFS at 94 dB SPL and the PDM driver's default gain is -10 dB.
const float MIC_DB_OFFSET = 130;

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
  float stats[2];       // IMU samples fused per second; degrees between where we think down is and true down
  float temperature;    // C
  float pressure;       // hPa
  float humidity;       // percent
  float color[4];       // red, green, blue, clear light counts
  float proximity;      // 0 far to 255 near
  float mic;            // approximate dB SPL
  uint8_t buttons;      // bit 0 = A, bit 1 = B
};
Readings readings;

char bleName[16];     // CLUE-xxxx, shown with the connection state on the top line
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
Adafruit_SHT31 sht30;
Adafruit_APDS9960 apds9960;
Adafruit_BMP280 bmp280;
MagCalibration magCalibration;

// The rotation that takes a vector in the device's frame to the world frame:
//   world = orientation.rotate(device)
// The device's frame is the motion chip's: y toward the USB plug, z down through the
// screen, x toward button A. The world frame is NED: x north, y east, z down. Identity
// is the board flat, screen up, USB pointing north.
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
  setupSensors();
  setupMic();
  setupImu();
  setupBle();
  readingsTop = display.getCursorY() + 8;
}

void loop() {
  static uint32_t lastQuaternionMs = 0, lastDebugMs = 0, lastSlowMs = 0, lastScreenMs = 0;

  // Data-ready stays high until we read, so also go when it is already high: a sample
  // that landed before the interrupt was attached never produces a rising edge.
  if (sampleReady || digitalRead(IMU_INTERRUPT_PIN)) {
    readImu();
  }
  if (every(1000 / QUATERNION_HZ, lastQuaternionMs)) {
    send(quaternionChar, readings.quaternion, sizeof(readings.quaternion));
    handleButtons();
  }
  if (every(1000 / DEBUG_HZ, lastDebugMs)) {
    readMag();
    readLightAndMic();
    send(vectorsChar, &readings.vectors, sizeof(readings.vectors));
    send(colorChar, readings.color, sizeof(readings.color));
    send(proximityChar, &readings.proximity, sizeof(readings.proximity));
    send(micChar, &readings.mic, sizeof(readings.mic));
  }
  if (every(1000 / SLOW_HZ, lastSlowMs)) {
    readEnvironment();
    fitMag();
    updateStats();
    send(temperatureChar, &readings.temperature, sizeof(readings.temperature));
    send(pressureChar, &readings.pressure, sizeof(readings.pressure));
    send(humidityChar, &readings.humidity, sizeof(readings.humidity));
    send(magFitChar, readings.magFit, sizeof(readings.magFit));
    send(statsChar, readings.stats, sizeof(readings.stats));
    showBleStatus();
  }
  if (every(1000 / SCREEN_LINES_HZ, lastScreenMs)) {
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

void updateStats() {
  readings.stats[0] = imuSamples * SLOW_HZ;
  imuSamples = 0;
}

void readMag() {
  sensors_event_t magEvent;
  lis3mdl.getEvent(&magEvent);
  magCalibration.add(magEvent.magnetic.v, readings.vectors.accel);
  for (int i = 0; i < 3; i++) {
    readings.vectors.mag[i] = magEvent.magnetic.v[i] - magCalibration.offset[i];
  }
  float magNed[3];
  toWorldFrame(magNed, readings.vectors.mag);
  lowPass(readings.vectors.magNed, magNed, 0.2);  // about the last half second at 10 Hz
}

// Refit the magnetometer's hard-iron offset. The fit only switches on once every face of
// the board has been pointed at the floor; until then the offset stays at zero.
void fitMag() {
  magCalibration.solve();
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
  readings.humidity = sht30.readHumidity();  // blocks for 20 ms, hence the slow rate
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

// Same as the factory demo: A beeps, B lights the white LEDs.
void handleButtons() {
  uint8_t buttons = (digitalRead(BUTTON_A_PIN) == LOW ? 1 : 0) | (digitalRead(BUTTON_B_PIN) == LOW ? 2 : 0);
  if (buttons != readings.buttons) {
    readings.buttons = buttons;
    send(buttonsChar, &buttons, sizeof(buttons));
  }
  if (buttons & 1) {
    tone(SPEAKER_PIN, 4000, 100);
  } else {
    tone(SPEAKER_PIN, 0);
  }
  digitalWrite(WHITE_LED_PIN, buttons & 2);
}

// Runs when a phone or web page writes three bytes, red green blue, to the pixel
// characteristic. The fixed length means any other size was already rejected.
void onPixelWrite(uint16_t connHandle, BLECharacteristic *chr, uint8_t *rgb, uint16_t len) {
  pixel.fill(pixel.Color(rgb[0], rgb[1], rgb[2]));
  pixel.show();
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

// The sensors are all on the board, so only a problem gets mentioned on the screen.
void setupSensors() {
  if (apds9960.begin()) {
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
  if (!sht30.begin(0x44)) {
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
  beginNotifyChar(quaternionChar, sizeof(readings.quaternion));
  beginNotifyChar(vectorsChar, sizeof(readings.vectors));
  beginNotifyChar(temperatureChar, sizeof(readings.temperature));
  beginNotifyChar(pressureChar, sizeof(readings.pressure));
  beginNotifyChar(humidityChar, sizeof(readings.humidity));
  beginNotifyChar(colorChar, sizeof(readings.color));
  beginNotifyChar(proximityChar, sizeof(readings.proximity));
  beginNotifyChar(micChar, sizeof(readings.mic));
  beginNotifyChar(buttonsChar, sizeof(readings.buttons));
  beginNotifyChar(magFitChar, sizeof(readings.magFit));
  beginNotifyChar(statsChar, sizeof(readings.stats));

  pixelChar.setProperties(CHR_PROPS_READ | CHR_PROPS_WRITE | CHR_PROPS_WRITE_WO_RESP);
  pixelChar.setFixedLen(3);
  pixelChar.setWriteCallback(onPixelWrite);
  pixelChar.begin();

  // Our 128-bit UUID nearly fills the advertising packet, so the name goes in the scan
  // response, which scanners ask for right after.
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addService(sensorService);
  Bluefruit.ScanResponse.addName();
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 32);  // every 20 ms, always, so reconnecting is quick
  Bluefruit.Advertising.start(0);             // 0 = keep advertising until connected
}

// A characteristic anyone can read or subscribe to, holding a fixed number of bytes.
void beginNotifyChar(BLECharacteristic &chr, int bytes) {
  chr.setProperties(CHR_PROPS_READ | CHR_PROPS_NOTIFY);
  chr.setFixedLen(bytes);
  chr.begin();
}

// Sends a new value to whoever subscribed. Bluefruit's own notify() waits up to 100 ms
// when the radio queue is full, which would stall the loop; this drops the packet instead.
bool send(BLECharacteristic &chr, const void *data, uint16_t len) {
  chr.write(data, len);  // keep the stored value current for anyone who reads instead
  if (!chr.notifyEnabled()) {
    return false;
  }
  ble_gatts_hvx_params_t hvx = {};
  hvx.handle = chr.handles().value_handle;
  hvx.type = BLE_GATT_HVX_NOTIFICATION;
  hvx.p_len = &len;
  hvx.p_data = (uint8_t *)data;
  return sd_ble_gatts_hvx(Bluefruit.connHandle(), &hvx) == NRF_SUCCESS;
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

// Shows a problem on the screen in red and keeps going.
void warn(const char *message) {
  Serial.println(message);
  display.setTextColor(ST77XX_RED);
  display.println(message);
}

void halt(const char *message) {
  warn(message);
  while (true) {
  }
}
