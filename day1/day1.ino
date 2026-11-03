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
const int IMU_INTERRUPT_PIN = 45;  // the IMU's INT1 is on P1.06, pin 45 in Arduino

// How often each group of sensors is read and sent, in packets per second.
const int QUATERNION_HZ = 30;   // the orientation, the one thing the page needs fast
const int DEBUG_HZ = 10;        // accel, gyro, mag, color, proximity, mic
const int ENVIRONMENT_HZ = 2;   // temperature, pressure, humidity: slow to read and slow to change
const int DISPLAY_LINES_HZ = 20;  // one line of the screen per tick, so 9 lines refresh every 0.45 s

// Turns the microphone's level relative to full scale into an approximate sound pressure
// level: the mic puts out -26 dBFS at 94 dB SPL, and the PDM driver's default gain is
// -10 dB, so SPL is about dBFS + 94 + 26 + 10. Good to a few dB.
const float MIC_DB_OFFSET = 130;

// How far toward the accelerometer's idea of "down" we move on each update.
// Small is smooth but slow to fix gyro drift; large is quick but jittery.
const float ACCEL_CORRECTION = 0.005;

// Our own BLE service. Every value is little-endian floats, except buttons and pixel
// which are bytes. orientation.html lists the same UUIDs.
BLEService sensorService("c10e0001-5a7e-4a6b-9f3e-0b6f2d9a1c01");
BLECharacteristic quaternionChar("c10e0002-5a7e-4a6b-9f3e-0b6f2d9a1c01");
BLECharacteristic vectorsChar("c10e0003-5a7e-4a6b-9f3e-0b6f2d9a1c01");  // accel, gyro, mag
BLECharacteristic temperatureChar("c10e0006-5a7e-4a6b-9f3e-0b6f2d9a1c01");
BLECharacteristic pressureChar("c10e0007-5a7e-4a6b-9f3e-0b6f2d9a1c01");
BLECharacteristic humidityChar("c10e0008-5a7e-4a6b-9f3e-0b6f2d9a1c01");
BLECharacteristic colorChar("c10e0009-5a7e-4a6b-9f3e-0b6f2d9a1c01");
BLECharacteristic micChar("c10e000a-5a7e-4a6b-9f3e-0b6f2d9a1c01");
BLECharacteristic buttonsChar("c10e000b-5a7e-4a6b-9f3e-0b6f2d9a1c01");
BLECharacteristic magFitChar("c10e000c-5a7e-4a6b-9f3e-0b6f2d9a1c01");
BLECharacteristic pixelChar("c10e000d-5a7e-4a6b-9f3e-0b6f2d9a1c01");  // writable: red, green, blue bytes
BLECharacteristic proximityChar("c10e000e-5a7e-4a6b-9f3e-0b6f2d9a1c01");
BLECharacteristic statsChar("c10e000f-5a7e-4a6b-9f3e-0b6f2d9a1c01");  // how the fusion is doing

// send() drops a packet when the radio queue is full rather than waiting, so the queue
// only needs to cover a burst of a few packets in one loop pass.
const int BLE_NOTIFY_QUEUE = 8;

// The motion vectors for the screen and the page, in one BLE packet: 15 floats. The NED
// ones are the raw readings rotated into the world frame by the orientation and averaged.
// If the fusion is right, accelNed sits at (0, 0, -9.8) and magNed holds still as the
// board turns.
struct Vectors {
  float accel[3];       // raw, m/s^2
  float gyro[3];        // raw with the bias removed, rad/s
  float mag[3];         // raw with the hard-iron offset removed, microtesla
  float accelNed[3];    // m/s^2
  float magNed[3];      // microtesla
};

// The latest reading from every sensor. The screen and BLE both read from here.
struct Readings {
  float quaternion[4];  // w, x, y, z
  Vectors vectors;
  float magFit[5];      // fitted offset x, y, z, sphere radius (microtesla), faces seen as bits +x -x +y -y +z -z
  float stats[2];       // IMU samples fused per second; angle in degrees between gyro's down and accel's down
  float temperature;    // C
  float pressure;       // hPa
  float humidity;       // percent
  float color[4];       // red, green, blue, clear light counts
  float proximity;      // 0 far to 255 near
  float mic;            // approximate dB SPL
  uint8_t buttons;      // bit 0 = A, bit 1 = B
};
Readings readings;

// The NeoPixel on the back of the board stays off until something writes a color over BLE.
uint8_t pixelColor[3] = {0, 0, 0};

int readingsTop = 0;  // screen row where the live readings start, below the startup lines
int imuSamples = 0;   // fused since the last stats update

Adafruit_ST7789 display(&SPI1, TFT_CS_PIN, TFT_DC_PIN, TFT_RESET_PIN);
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

// Rotation from the board's frame to the world frame. The board's frame is the motion
// chip's own: y toward the USB plug, z down through the screen, x to the left. The world
// frame is NED: x north, y east, z down. The identity rotation is the board lying flat,
// screen up, with the USB plug pointing north. Lying flat the accelerometer reads -g on
// z, since it measures the push holding the board up, not gravity itself.
Quaternion orientation;

// The gyro reads a small constant rate even when still. Measured at startup and subtracted.
float gyroBias[3] = {0, 0, 0};

// Set by the IMU's data-ready interrupt, cleared when loop() consumes the sample.
// The CPU cycle counter gives us a precise timestamp for when the sample was taken.
volatile bool sampleReady = false;
volatile uint32_t sampleCycles = 0;

// Running sums of the microphone samples since last checked, kept by the PDM callback,
// for the RMS level. The sum of squares needs 64 bits.
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
  display.setTextWrap(false);
  readingsTop = display.getCursorY() + 8;  // just below the BLE name and any warnings
}

void loop() {
  static uint32_t lastQuaternionMs = 0, lastDebugMs = 0, lastEnvironmentMs = 0, lastShowMs = 0;

  // Data-ready stays high until we read, so also go when it is already high: a sample
  // that landed before we attached the interrupt never produces a rising edge.
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
  if (every(1000 / ENVIRONMENT_HZ, lastEnvironmentMs)) {
    readEnvironment();
    fitMag();
    updateStats();
    send(magFitChar, readings.magFit, sizeof(readings.magFit));
    send(statsChar, readings.stats, sizeof(readings.stats));
    send(temperatureChar, &readings.temperature, sizeof(readings.temperature));
    send(pressureChar, &readings.pressure, sizeof(readings.pressure));
    send(humidityChar, &readings.humidity, sizeof(readings.humidity));
  }
  if (every(1000 / DISPLAY_LINES_HZ, lastShowMs)) {
    showNextLine();
    updatePixel();
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

  sensors_event_t event[3];  // accel, gyro, temperature
  imu->getEvent(&event[0], &event[1], &event[2]);
  float accel[3], gyro[3];
  copyVector(accel, event[0].acceleration);
  copyVector(gyro, event[1].gyro);
  for (int i = 0; i < 3; i++) {
    gyro[i] -= gyroBias[i];
  }

  // How far the board turned since the last sample. Gyro is in radians per second, so
  // rate * dt is the rotation.
  Quaternion turn = Quaternion::from_axis_angle(gyro[0] * dt, gyro[1] * dt, gyro[2] * dt);
  updateOrientation(turn, accel);
  readings.quaternion[0] = orientation.a;
  readings.quaternion[1] = orientation.b;
  readings.quaternion[2] = orientation.c;
  readings.quaternion[3] = orientation.d;

  // For the page and screen: the raw readings, and the accel in the world frame where
  // gravity is a constant, averaged over about the last 50 ms (0.05 per sample at 416 Hz).
  copyVector(readings.vectors.accel, accel);
  copyVector(readings.vectors.gyro, gyro);
  float accelNed[3];
  toWorldFrame(accelNed, accel);
  lowPass(readings.vectors.accelNed, accelNed, 0.05);
}

// Rotates a vector from the board's frame into the world frame using the orientation.
void toWorldFrame(float dest[3], const float v[3]) {
  Quaternion world = orientation.rotate(Quaternion(v[0], v[1], v[2]));
  dest[0] = world.b;
  dest[1] = world.c;
  dest[2] = world.d;
}

// Moves a smoothed vector the given fraction of the way toward the latest reading.
void lowPass(float filtered[3], const float latest[3], float fraction) {
  for (int i = 0; i < 3; i++) {
    filtered[i] += (latest[i] - filtered[i]) * fraction;
  }
}

// The gyro says how we turned since last time; the accelerometer says which way
// is down. Apply the turn fully, then nudge the result toward the accelerometer.
void updateOrientation(const Quaternion &turn, const float accel[3]) {
  orientation *= turn;
  orientation.normalize();

  // At rest the accelerometer only feels the push holding the board up, so down is the
  // opposite of what it reads. In the world frame down is simply +z.
  Quaternion measuredDown(-accel[0], -accel[1], -accel[2]);
  measuredDown.normalize();
  Quaternion expectedDown = orientation.conj().rotate(Quaternion(0, 0, 1));

  // Rotate a fraction of the way from where we think down is to where the accelerometer says it is.
  Quaternion correction = measuredDown.rotation_between_vectors(expectedDown);
  orientation *= correction.fractional(ACCEL_CORRECTION);
  readings.stats[1] = acos(constrain(measuredDown.dot_product(expectedDown), -1, 1)) * 180 / PI;
}

void updateStats() {
  readings.stats[0] = imuSamples * ENVIRONMENT_HZ;
  imuSamples = 0;
}

void readMag() {
  sensors_event_t mag;
  lis3mdl.getEvent(&mag);
  magCalibration.add(mag.magnetic.v, readings.vectors.accel);
  copyVector(readings.vectors.mag, mag.magnetic);
  for (int i = 0; i < 3; i++) {
    readings.vectors.mag[i] -= magCalibration.offset[i];
  }

  // Earth's field is constant in the world frame; average it there over about half a
  // second (0.2 per reading at 10 Hz).
  float magNed[3];
  toWorldFrame(magNed, readings.vectors.mag);
  lowPass(readings.vectors.magNed, magNed, 0.2);
}

// Refit the hard-iron offset from the readings so far. The fit only switches on once
// every face of the board has been pointed at the floor.
void fitMag() {
  magCalibration.solve();
  for (int i = 0; i < 3; i++) {
    readings.magFit[i] = magCalibration.offset[i];
  }
  readings.magFit[3] = magCalibration.radius;
  readings.magFit[4] = magCalibration.directionsSeen();
}

// ---- The other sensors ----

void readEnvironment() {
  readings.temperature = bmp280.readTemperature();
  readings.pressure = bmp280.readPressure() / 100;
  readings.humidity = sht30.readHumidity();
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

// The RMS level of the samples since the last call, converted to decibels.
float micDecibels() {
  noInterrupts();  // so the PDM callback cannot change the sums while we copy and reset them
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

void updatePixel() {
  pixel.fill(pixel.Color(pixelColor[0], pixelColor[1], pixelColor[2]));
  pixel.show();
}

void setPixel(uint8_t red, uint8_t green, uint8_t blue) {
  pixelColor[0] = red;
  pixelColor[1] = green;
  pixelColor[2] = blue;
  pixelChar.write(pixelColor, sizeof(pixelColor));  // so reading the characteristic shows the current color
}

// Runs when a phone or web page writes the pixel characteristic. The fixed length
// means the SoftDevice already rejected any write that is not exactly three bytes.
void onPixelWrite(uint16_t connHandle, BLECharacteristic *chr, uint8_t *data, uint16_t len) {
  setPixel(data[0], data[1], data[2]);
}

// ---- Setup ----

void setupDisplay() {
  display.init(240, 240);
  display.setRotation(1);
  display.fillScreen(ST77XX_BLACK);
  pinMode(TFT_BACKLIGHT_PIN, OUTPUT);
  digitalWrite(TFT_BACKLIGHT_PIN, HIGH);
  display.setCursor(0, 0);
  display.setTextWrap(true);
  display.setTextSize(2);
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
  measureGyroBias();
  attachInterrupt(IMU_INTERRUPT_PIN, onImuDataReady, RISING);
}

// Average half a second of gyro readings while the board sits still after reset.
void measureGyroBias() {
  const int samples = 200;
  sensors_event_t accel, gyro, temp;
  for (int n = 0; n < samples; n++) {
    while (!digitalRead(IMU_INTERRUPT_PIN)) {
    }
    imu->getEvent(&accel, &gyro, &temp);
    for (int i = 0; i < 3; i++) {
      gyroBias[i] += gyro.gyro.v[i] / samples;
    }
  }
}

void setupBle() {
  // (max MTU, max connection event length in 1.25 ms units, notify queue, write queue)
  Bluefruit.configPrphConn(247, 100, BLE_NOTIFY_QUEUE, 1);
  Bluefruit.begin();
  Bluefruit.Periph.setConnectCallback(onConnect);

  // Give each CLUE its own name so a room full of them is easy to tell apart.
  char name[16];
  ble_gap_addr_t addr = Bluefruit.getAddr();
  snprintf(name, sizeof(name), "CLUE-%02X%02X", addr.addr[1], addr.addr[0]);
  Bluefruit.setName(name);
  display.setTextColor(ST77XX_WHITE);
  display.print("BLE: ");
  display.println(name);

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
  pixelChar.setFixedLen(sizeof(pixelColor));
  pixelChar.setWriteCallback(onPixelWrite);
  pixelChar.begin();

  // A 128-bit UUID nearly fills the 31-byte advertising packet, leaving only 8 characters
  // for the name, so the name goes in the scan response, which scanners ask for right after.
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addService(sensorService);
  Bluefruit.ScanResponse.addName();
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.start(0);  // 0 = keep advertising until connected
}

// A characteristic anyone can read or subscribe to, holding a fixed number of bytes.
void beginNotifyChar(BLECharacteristic &chr, int bytes) {
  chr.setProperties(CHR_PROPS_READ | CHR_PROPS_NOTIFY);
  chr.setFixedLen(bytes);
  chr.begin();
}

// Ask the laptop or phone for the fast BLE 5 radio settings: the 2 Mbit PHY, link-layer
// packets big enough to carry the vectors packet whole, and an MTU to match. Each one is
// a request that the other side may turn down. The quaternion fits the default MTU anyway.
void onConnect(uint16_t connHandle) {
  BLEConnection *conn = Bluefruit.Connection(connHandle);
  conn->requestPHY(BLE_GAP_PHY_2MBPS);
  conn->requestDataLengthUpdate();
  conn->requestMtuExchange(sizeof(Vectors) + 3);  // +3 for the notification header
}

// Sends a characteristic's new value to whoever subscribed to it. Bluefruit's own notify()
// waits up to 100 ms for room in the radio queue, which would stall this whole loop whenever
// the laptop is slow to take packets. This version just skips the packet instead; the next
// one carries a fresher reading anyway.
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

// Drawing the whole screen at once takes long enough to hold up BLE packets and IMU
// samples, so each call draws just one of the nine lines.
void showNextLine() {
  static int line = 0;
  display.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  display.setCursor(0, readingsTop + line * 16);
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
  line = (line + 1) % 9;
}

// Trailing spaces on each line erase leftovers from a previous longer value.
void showValue(const char *label, float value, int decimals, const char *unit) {
  display.print(label);
  display.print(value, decimals);
  display.print(unit);
  display.println("         ");
}

void showVector(const char *label, const float v[3]) {
  display.print(label);
  display.print(v[0], 1);
  display.print(",");
  display.print(v[1], 1);
  display.print(",");
  display.print(v[2], 1);
  display.println("         ");
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

void copyVector(float dest[3], const sensors_vec_t &v) {
  dest[0] = v.x;
  dest[1] = v.y;
  dest[2] = v.z;
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
