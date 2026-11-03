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

// How often each group of sensors is read and sent, in notifications per second.
const int MOTION_HZ = 30;       // quaternion, accel, gyro, mag
const int LIGHT_MIC_HZ = 10;    // color, proximity, mic
const int ENVIRONMENT_HZ = 2;   // temperature, pressure, humidity: slow to read and slow to change
const int DISPLAY_HZ = 5;

// How far toward the accelerometer's idea of "down" we move on each update.
// Small is smooth but slow to fix gyro drift; large is quick but jittery.
const float ACCEL_CORRECTION = 0.005;

// Our own BLE service, one characteristic per sensor. Every value is little-endian
// floats, except buttons and pixel which are bytes. orientation.html lists the same UUIDs.
BLEService sensorService("c10e0001-5a7e-4a6b-9f3e-0b6f2d9a1c01");
BLECharacteristic quaternionChar("c10e0002-5a7e-4a6b-9f3e-0b6f2d9a1c01");
BLECharacteristic accelChar("c10e0003-5a7e-4a6b-9f3e-0b6f2d9a1c01");
BLECharacteristic gyroChar("c10e0004-5a7e-4a6b-9f3e-0b6f2d9a1c01");
BLECharacteristic magChar("c10e0005-5a7e-4a6b-9f3e-0b6f2d9a1c01");
BLECharacteristic temperatureChar("c10e0006-5a7e-4a6b-9f3e-0b6f2d9a1c01");
BLECharacteristic pressureChar("c10e0007-5a7e-4a6b-9f3e-0b6f2d9a1c01");
BLECharacteristic humidityChar("c10e0008-5a7e-4a6b-9f3e-0b6f2d9a1c01");
BLECharacteristic colorChar("c10e0009-5a7e-4a6b-9f3e-0b6f2d9a1c01");
BLECharacteristic micChar("c10e000a-5a7e-4a6b-9f3e-0b6f2d9a1c01");
BLECharacteristic buttonsChar("c10e000b-5a7e-4a6b-9f3e-0b6f2d9a1c01");
BLECharacteristic magFitChar("c10e000c-5a7e-4a6b-9f3e-0b6f2d9a1c01");
BLECharacteristic pixelChar("c10e000d-5a7e-4a6b-9f3e-0b6f2d9a1c01");  // writable: red, green, blue bytes
BLECharacteristic proximityChar("c10e000e-5a7e-4a6b-9f3e-0b6f2d9a1c01");

// We send about 160 notifications a second across all characteristics. The SoftDevice
// default queue holds just one, and notify() stalls for up to 100 ms when it is full.
const int BLE_NOTIFY_QUEUE = 16;

// The latest reading from every sensor. The screen and BLE both read from here.
struct Readings {
  float quaternion[4];  // w, x, y, z
  float accel[3];       // m/s^2
  float gyro[3];        // rad/s
  float mag[3];         // microtesla, with the hard-iron offset removed
  float magFit[5];      // fitted offset x, y, z, sphere radius (microtesla), octants covered of 8
  float temperature;    // C
  float pressure;       // hPa
  float humidity;       // percent
  float color[4];       // red, green, blue, clear light counts
  float proximity;      // 0 far to 255 near
  float mic;            // peak-to-peak amplitude
  uint8_t buttons;      // bit 0 = A, bit 1 = B
};
Readings readings;

// The NeoPixel on the back of the board stays off until something writes a color over BLE.
uint8_t pixelColor[3] = {0, 0, 0};

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

// Rotation from the board's frame (x right, y toward USB, z out of the screen)
// to the world frame, where z is up.
Quaternion orientation;

// Set by the IMU's data-ready interrupt, cleared when loop() consumes the sample.
// The CPU cycle counter gives us a precise timestamp for when the sample was taken.
volatile bool sampleReady = false;
volatile uint32_t sampleCycles = 0;

// Quietest and loudest microphone sample since last checked, kept by the PDM callback.
volatile int16_t micMin = INT16_MAX, micMax = INT16_MIN;
int16_t micSamples[256];

void setup() {
  Serial.begin(115200);
  dwt_enable();  // start the CPU cycle counter
  pinMode(WHITE_LED_PIN, OUTPUT);
  pinMode(BUTTON_A_PIN, INPUT_PULLUP);
  pinMode(BUTTON_B_PIN, INPUT_PULLUP);
  pixel.begin();

  setupDisplay();
  checkSensors();
  setupMic();
  setupImu();
  setupBle();
  display.setTextWrap(false);
}

void loop() {
  static uint32_t lastMotionMs = 0, lastLightMicMs = 0, lastEnvironmentMs = 0, lastShowMs = 0;

  // Data-ready stays high until we read, so also go when it is already high: a sample
  // that landed before we attached the interrupt never produces a rising edge.
  if (sampleReady || digitalRead(IMU_INTERRUPT_PIN)) {
    readImu();
  }
  if (every(1000 / MOTION_HZ, lastMotionMs)) {
    readMag();
    quaternionChar.notify(readings.quaternion, sizeof(readings.quaternion));
    accelChar.notify(readings.accel, sizeof(readings.accel));
    gyroChar.notify(readings.gyro, sizeof(readings.gyro));
    magChar.notify(readings.mag, sizeof(readings.mag));
    handleButtons();
  }
  if (every(1000 / LIGHT_MIC_HZ, lastLightMicMs)) {
    readLightAndMic();
    colorChar.notify(readings.color, sizeof(readings.color));
    proximityChar.notify32(readings.proximity);
    micChar.notify32(readings.mic);
  }
  if (every(1000 / ENVIRONMENT_HZ, lastEnvironmentMs)) {
    readEnvironment();
    fitMag();
    magFitChar.notify(readings.magFit, sizeof(readings.magFit));
    temperatureChar.notify32(readings.temperature);
    pressureChar.notify32(readings.pressure);
    humidityChar.notify32(readings.humidity);
  }
  if (every(1000 / DISPLAY_HZ, lastShowMs)) {
    showReadings();
    updatePixel();
  }
  analogWrite(LED_BUILTIN, 255 - millis() % 256);  // the factory demo's red LED pulse
}

// ---- Motion sensors and orientation ----

void onImuDataReady() {
  sampleCycles = DWT->CYCCNT;
  sampleReady = true;
}

void readImu() {
  static uint32_t lastSampleCycles = DWT->CYCCNT;
  sampleReady = false;
  float dt = (float)(sampleCycles - lastSampleCycles) / SystemCoreClock;
  lastSampleCycles = sampleCycles;

  sensors_event_t accel, gyro, temp;
  imu->getEvent(&accel, &gyro, &temp);
  updateOrientation(accel, gyro, dt);

  copyVector(readings.accel, accel.acceleration);
  copyVector(readings.gyro, gyro.gyro);
  readings.quaternion[0] = orientation.a;
  readings.quaternion[1] = orientation.b;
  readings.quaternion[2] = orientation.c;
  readings.quaternion[3] = orientation.d;
}

// The gyro says how we turned since last time; the accelerometer says which way
// is down. Apply the gyro fully, then nudge the result toward the accelerometer.
void updateOrientation(const sensors_event_t &accel, const sensors_event_t &gyro, float dt) {
  // Gyro is in radians per second, so rate * dt is the rotation since last update.
  orientation *= Quaternion::from_euler_rotation_approx(gyro.gyro.x * dt, gyro.gyro.y * dt, gyro.gyro.z * dt);
  orientation.normalize();

  // At rest the accelerometer only feels gravity, which points up in the board's frame.
  Quaternion measuredUp(accel.acceleration.x, accel.acceleration.y, accel.acceleration.z);
  measuredUp.normalize();
  Quaternion expectedUp = orientation.conj().rotate(Quaternion(0, 0, 1));

  // Rotate a fraction of the way from where we think up is to where the accelerometer says it is.
  Quaternion correction = measuredUp.rotation_between_vectors(expectedUp);
  orientation *= correction.fractional(ACCEL_CORRECTION);
}

void readMag() {
  sensors_event_t mag;
  lis3mdl.getEvent(&mag);
  magCalibration.add(mag.magnetic.v, readings.accel);
  copyVector(readings.mag, mag.magnetic);
  for (int i = 0; i < 3; i++) {
    readings.mag[i] -= magCalibration.offset[i];
  }
}

// Refit the hard-iron offset from the readings so far. The fit only switches on once
// every corner of the board has been tipped toward the floor, face up and face down.
void fitMag() {
  magCalibration.solve();
  for (int i = 0; i < 3; i++) {
    readings.magFit[i] = magCalibration.offset[i];
  }
  readings.magFit[3] = magCalibration.radius;
  readings.magFit[4] = magCalibration.octantsCovered();
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

  readings.mic = micMax - micMin;
  micMin = INT16_MAX;  // so the next sample becomes both the min and the max
  micMax = INT16_MIN;
}

// Called by the PDM library whenever a chunk of microphone samples is ready.
void onMicData() {
  int count = PDM.read(micSamples, sizeof(micSamples)) / sizeof(micSamples[0]);
  for (int i = 0; i < count; i++) {
    if (micSamples[i] < micMin) {
      micMin = micSamples[i];
    }
    if (micSamples[i] > micMax) {
      micMax = micSamples[i];
    }
  }
}

// Same as the factory demo: A beeps, B lights the white LEDs.
void handleButtons() {
  uint8_t buttons = (digitalRead(BUTTON_A_PIN) == LOW ? 1 : 0) | (digitalRead(BUTTON_B_PIN) == LOW ? 2 : 0);
  if (buttons != readings.buttons) {
    readings.buttons = buttons;
    buttonsChar.notify8(buttons);
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

void checkSensors() {
  display.setTextColor(ST77XX_WHITE);
  display.println("Sensors Found: ");

  bool apdsFound = apds9960.begin();
  if (apdsFound) {
    apds9960.enableColor(true);
    apds9960.enableProximity(true);
  }
  reportSensor("APDS9960 ", apdsFound);

  if (lsm6ds33.begin_I2C()) {
    imu = &lsm6ds33;
  } else if (lsm6ds3trc.begin_I2C()) {
    imu = &lsm6ds3trc;
  }
  reportSensor("LSM6DS3x ", imu != NULL);
  display.println();

  reportSensor("LIS3MDL ", lis3mdl.begin_I2C());
  reportSensor("SHT30 ", sht30.begin(0x44));
  reportSensor("BMP280", bmp280.begin());
  display.println();

  Wire.setClock(400000);  // every chip supports fast mode I2C; their begin() calls left it at 100 kHz
}

// Prints a sensor's name on the startup screen: green if it answered, red if not.
void reportSensor(const char *name, bool found) {
  Serial.print(name);
  Serial.println(found ? "OK" : "not found");
  display.setTextColor(found ? ST77XX_GREEN : ST77XX_RED);
  display.print(name);
}

void setupMic() {
  PDM.onReceive(onMicData);
  if (!PDM.begin(1, 16000)) {
    Serial.println("Failed to start PDM");
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
  beginNotifyChar(accelChar, sizeof(readings.accel));
  beginNotifyChar(gyroChar, sizeof(readings.gyro));
  beginNotifyChar(magChar, sizeof(readings.mag));
  beginNotifyChar(temperatureChar, sizeof(readings.temperature));
  beginNotifyChar(pressureChar, sizeof(readings.pressure));
  beginNotifyChar(humidityChar, sizeof(readings.humidity));
  beginNotifyChar(colorChar, sizeof(readings.color));
  beginNotifyChar(proximityChar, sizeof(readings.proximity));
  beginNotifyChar(micChar, sizeof(readings.mic));
  beginNotifyChar(buttonsChar, sizeof(readings.buttons));
  beginNotifyChar(magFitChar, sizeof(readings.magFit));

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

// Ask the laptop or phone for the fast BLE 5 radio settings: the 2 Mbit PHY and
// bigger link-layer packets. Each one is a request that the other side may turn down.
void onConnect(uint16_t connHandle) {
  BLEConnection *conn = Bluefruit.Connection(connHandle);
  conn->requestPHY(BLE_GAP_PHY_2MBPS);
  conn->requestDataLengthUpdate();
}

// ---- Screen, laid out like the factory demo ----

void showReadings() {
  display.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  display.setCursor(0, 100);
  showValue("Temp: ", readings.temperature, 2, " C");
  showValue("Baro: ", readings.pressure, 2, " hPa");
  showValue("Humid: ", readings.humidity, 2, " %");
  showValue("Light: ", readings.color[3], 0, "");
  showVector("Accel:", readings.accel);
  showVector("Gyro:", readings.gyro);
  showVector("Mag:", readings.mag);
  showValue("Mic: ", readings.mic, 0, "");
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

void halt(const char *message) {
  Serial.println(message);
  display.setTextColor(ST77XX_RED);
  display.println(message);
  while (true) {
  }
}
