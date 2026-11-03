// Day 1: the CLUE factory demo, plus an orientation fused from the motion
// sensors and every reading streamed over Bluetooth LE to orientation.html.
//
// Libraries (Arduino Library Manager unless noted): Adafruit Arcada, Adafruit LSM6DS,
//   Adafruit LIS3MDL, Adafruit SHT31, Adafruit APDS9960, Adafruit BMP280, and the
//   Adafruit nRF52 board package. Quaternion: https://github.com/carrino/Quaternion

#include <Adafruit_Arcada.h>
#include <Adafruit_SPIFlash.h>
#include <Adafruit_LSM6DS33.h>
#include <Adafruit_LSM6DS3TRC.h>
#include <Adafruit_LIS3MDL.h>
#include <Adafruit_SHT31.h>
#include <Adafruit_APDS9960.h>
#include <Adafruit_BMP280.h>
#include <PDM.h>
#include <Quaternion.h>
#include <bluefruit.h>

const int WHITE_LED_PIN = 43;
const int IMU_INTERRUPT_PIN = 45;  // the CLUE wires the IMU's INT1 to P1.06, pin 45 in Arduino
const int SEND_HZ = 30;            // packets to the web page per second
const int DISPLAY_HZ = 5;
const int ENVIRONMENT_HZ = 2;      // temperature, pressure and humidity are slow to read and slow to change

// How far toward the accelerometer's idea of "down" we move on each update.
// Small is smooth but slow to fix gyro drift; large is quick but jittery.
const float ACCEL_CORRECTION = 0.005;

// Our own BLE service. orientation.html looks for these same UUIDs.
const char *ORIENTATION_SERVICE_UUID = "c10e0001-5a7e-4a6b-9f3e-0b6f2d9a1c01";
const char *SENSORS_UUID = "c10e0002-5a7e-4a6b-9f3e-0b6f2d9a1c01";

// The latest reading from every sensor. One BLE notification carries the whole thing.
// It is all floats, so there is no padding and the web page reads it as 22
// little-endian floats in this order.
struct SensorPacket {
  float quaternion[4];  // w, x, y, z
  float accel[3];       // m/s^2
  float gyro[3];        // rad/s
  float mag[3];         // microtesla
  float temperature;    // C
  float pressure;       // hPa
  float humidity;       // percent
  float color[4];       // red, green, blue, clear light counts
  float mic;            // peak-to-peak amplitude
  float buttons;        // bit 0 = A, bit 1 = B
};

Adafruit_Arcada arcada;
extern Adafruit_SPIFlash Arcada_QSPI_Flash;

// CLUE boards ship with one of two nearly identical motion chips.
Adafruit_LSM6DS33 lsm6ds33;
Adafruit_LSM6DS3TRC lsm6ds3trc;
Adafruit_LSM6DS *imu = NULL;
Adafruit_LIS3MDL lis3mdl;
Adafruit_SHT31 sht30;
Adafruit_APDS9960 apds9960;
Adafruit_BMP280 bmp280;

BLEService orientationService(ORIENTATION_SERVICE_UUID);
BLECharacteristic sensorsChar(SENSORS_UUID, CHR_PROPS_READ | CHR_PROPS_NOTIFY, sizeof(SensorPacket), true);

SensorPacket packet;

// Rotation from the board's frame (x right, y toward USB, z out of the screen)
// to the world frame, where z is up.
Quaternion orientation;

// Set by the IMU's data-ready interrupt, cleared when loop() consumes the sample.
// The CPU cycle counter gives us a precise timestamp for when the sample was taken.
volatile bool sampleReady = false;
volatile uint32_t sampleCycles = 0;

// Quietest and loudest microphone sample since the last packet, kept by the PDM callback.
volatile int16_t micMin = INT16_MAX, micMax = INT16_MIN;
int16_t micSamples[256];

void setup() {
  Serial.begin(115200);
  dwt_enable();  // start the CPU cycle counter
  pinMode(WHITE_LED_PIN, OUTPUT);

  setupDisplay();
  checkFlash();
  checkSensors();
  setupMic();
  setupImu();
  setupBle();

  arcada.timerCallback(1000, pulseRedLed);
  arcada.display->setTextWrap(false);
}

void loop() {
  static uint32_t lastSendMs = 0, lastShowMs = 0, lastEnvironmentMs = 0;

  // Data-ready stays high until we read, so also go when it is already high: a sample
  // that landed before we attached the interrupt never produces a rising edge.
  if (sampleReady || digitalRead(IMU_INTERRUPT_PIN)) {
    readImu();
  }
  if (every(1000 / ENVIRONMENT_HZ, lastEnvironmentMs)) {
    readEnvironment();
  }
  if (every(1000 / SEND_HZ, lastSendMs)) {
    readOtherSensors();
    handleButtons();
    sensorsChar.notify(&packet, sizeof(packet));
  }
  if (every(1000 / DISPLAY_HZ, lastShowMs)) {
    showReadings();
    stepRainbow();
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
  float dt = (float)(sampleCycles - lastSampleCycles) / SystemCoreClock;
  lastSampleCycles = sampleCycles;

  sensors_event_t accel, gyro, temp;
  imu->getEvent(&accel, &gyro, &temp);
  updateOrientation(accel, gyro, dt);

  copyVector(packet.accel, accel.acceleration);
  copyVector(packet.gyro, gyro.gyro);
  packet.quaternion[0] = orientation.a;
  packet.quaternion[1] = orientation.b;
  packet.quaternion[2] = orientation.c;
  packet.quaternion[3] = orientation.d;
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

// ---- The other sensors ----

void readEnvironment() {
  packet.temperature = bmp280.readTemperature();
  packet.pressure = bmp280.readPressure() / 100;
  packet.humidity = sht30.readHumidity();
}

void readOtherSensors() {
  sensors_event_t mag;
  lis3mdl.getEvent(&mag);
  copyVector(packet.mag, mag.magnetic);

  if (apds9960.colorDataReady()) {
    uint16_t r, g, b, c;
    apds9960.getColorData(&r, &g, &b, &c);
    packet.color[0] = r;
    packet.color[1] = g;
    packet.color[2] = b;
    packet.color[3] = c;
  }

  packet.mic = micMax - micMin;
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
  uint8_t buttons = arcada.readButtons();
  packet.buttons = buttons & (ARCADA_BUTTONMASK_A | ARCADA_BUTTONMASK_B);

  if (buttons & ARCADA_BUTTONMASK_A) {
    tone(ARCADA_AUDIO_OUT, 4000, 100);
  } else {
    tone(ARCADA_AUDIO_OUT, 0);
  }
  digitalWrite(WHITE_LED_PIN, buttons & ARCADA_BUTTONMASK_B);
}

void stepRainbow() {
  static uint16_t hue = 0;
  hue += 1280;
  arcada.pixels.fill(arcada.pixels.ColorHSV(hue));
  arcada.pixels.show();
}

void pulseRedLed() {
  static uint8_t brightness = 0;
  analogWrite(LED_BUILTIN, brightness--);
}

// ---- Setup ----

void setupDisplay() {
  if (!arcada.arcadaBegin()) {
    halt("Arcada failed");
  }
  arcada.displayBegin();
  arcada.setBacklight(255);
  arcada.display->setCursor(0, 0);
  arcada.display->setTextWrap(true);
  arcada.display->setTextSize(2);
}

void checkFlash() {
  if (Arcada_QSPI_Flash.begin()) {
    arcada.display->setTextColor(ARCADA_GREEN);
    arcada.display->print("QSPI JEDEC: 0x");
    arcada.display->println(Arcada_QSPI_Flash.getJEDECID(), HEX);
  } else {
    arcada.display->setTextColor(ARCADA_RED);
    arcada.display->println("QSPI Flash FAIL");
  }

  if (arcada.filesysBegin()) {
    arcada.display->setTextColor(ARCADA_GREEN);
    arcada.display->println("Filesystem OK");
  } else {
    arcada.display->setTextColor(ARCADA_YELLOW);
    arcada.display->println("Filesystem not found");
  }
}

void checkSensors() {
  arcada.display->setTextColor(ARCADA_WHITE);
  arcada.display->println("Sensors Found: ");

  bool apdsFound = apds9960.begin();
  if (apdsFound) {
    apds9960.enableColor(true);
  }
  reportSensor("APDS9960 ", apdsFound);

  if (lsm6ds33.begin_I2C()) {
    imu = &lsm6ds33;
  } else if (lsm6ds3trc.begin_I2C()) {
    imu = &lsm6ds3trc;
  }
  reportSensor("LSM6DS3x ", imu != NULL);
  arcada.display->println();

  reportSensor("LIS3MDL ", lis3mdl.begin_I2C());
  reportSensor("SHT30 ", sht30.begin(0x44));
  reportSensor("BMP280", bmp280.begin());
  arcada.display->println();

  Wire.setClock(400000);  // every chip supports fast mode I2C; their begin() calls left it at 100 kHz
}

// Prints a sensor's name on the startup screen: green if it answered, red if not.
void reportSensor(const char *name, bool found) {
  Serial.printf("%s%s\n", name, found ? "OK" : "not found");
  arcada.display->setTextColor(found ? ARCADA_GREEN : ARCADA_RED);
  arcada.display->print(name);
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
  Bluefruit.configPrphBandwidth(BANDWIDTH_MAX);  // allow a big enough MTU for the whole packet
  Bluefruit.begin();
  Bluefruit.Periph.setConnectCallback(onConnect);

  // Give each CLUE its own name so a room full of them is easy to tell apart.
  char name[16];
  ble_gap_addr_t addr = Bluefruit.getAddr();
  snprintf(name, sizeof(name), "CLUE-%02X%02X", addr.addr[1], addr.addr[0]);
  Bluefruit.setName(name);
  arcada.display->setTextColor(ARCADA_WHITE);
  arcada.display->print("BLE: ");
  arcada.display->println(name);

  orientationService.begin();
  sensorsChar.begin();  // characteristics join the most recently begun service

  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addService(orientationService);
  Bluefruit.Advertising.addName();
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.start(0);  // 0 = keep advertising until connected
}

// Ask the laptop or phone for the fast BLE 5 radio settings: the 2 Mbit PHY, link-layer
// packets big enough to carry our packet whole, and a matching MTU. Each one is a
// request that the other side is free to turn down.
void onConnect(uint16_t connHandle) {
  BLEConnection *conn = Bluefruit.Connection(connHandle);
  conn->requestPHY(BLE_GAP_PHY_2MBPS);
  conn->requestDataLengthUpdate();
  conn->requestMtuExchange(sizeof(SensorPacket) + 3);  // +3 for the ATT notification header
}

// ---- Screen, laid out like the factory demo ----

void showReadings() {
  arcada.display->setTextColor(ARCADA_WHITE, ARCADA_BLACK);
  arcada.display->setCursor(0, 100);
  showValue("Temp: ", packet.temperature, 2, " C");
  showValue("Baro: ", packet.pressure, 2, " hPa");
  showValue("Humid: ", packet.humidity, 2, " %");
  showValue("Light: ", packet.color[3], 0, "");
  showVector("Accel:", packet.accel);
  showVector("Gyro:", packet.gyro);
  showVector("Mag:", packet.mag);
  showValue("Mic: ", packet.mic, 0, "");
}

// Trailing spaces on each line erase leftovers from a previous longer value.
void showValue(const char *label, float value, int decimals, const char *unit) {
  arcada.display->print(label);
  arcada.display->print(value, decimals);
  arcada.display->print(unit);
  arcada.display->println("         ");
}

void showVector(const char *label, const float v[3]) {
  arcada.display->print(label);
  arcada.display->print(v[0], 1);
  arcada.display->print(",");
  arcada.display->print(v[1], 1);
  arcada.display->print(",");
  arcada.display->print(v[2], 1);
  arcada.display->println("         ");
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
  arcada.display->setTextColor(ARCADA_RED);
  arcada.display->println(message);
  while (true) {
  }
}
