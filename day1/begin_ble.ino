// Day 1: read the CLUE's motion sensors, fuse them into an orientation, and
// stream everything over Bluetooth LE to orientation.html.
//
// Libraries (Arduino Library Manager unless noted):
//   Adafruit Arcada, Adafruit LSM6DS, Adafruit LIS3MDL, and the Adafruit nRF52 board package
//   Quaternion: https://github.com/carrino/Quaternion

#include <Adafruit_Arcada.h>
#include <Adafruit_LSM6DS33.h>
#include <Adafruit_LSM6DS3TRC.h>
#include <Adafruit_LIS3MDL.h>
#include <Quaternion.h>
#include <bluefruit.h>

const int IMU_INTERRUPT_PIN = 45;  // the CLUE wires the IMU's INT1 to P1.06, pin 45 in Arduino
const int SEND_HZ = 30;            // packets to the web page per second
const int DISPLAY_HZ = 5;

// How far toward the accelerometer's idea of "down" we move on each update.
// Small is smooth but slow to fix gyro drift; large is quick but jittery.
const float ACCEL_CORRECTION = 0.005;

// Our own BLE service. orientation.html looks for these same UUIDs.
const char *ORIENTATION_SERVICE_UUID = "c10e0001-5a7e-4a6b-9f3e-0b6f2d9a1c01";
const char *SENSORS_UUID = "c10e0002-5a7e-4a6b-9f3e-0b6f2d9a1c01";

// One BLE notification carries this whole snapshot. It is all floats, so there is
// no padding and the web page can read it as 13 little-endian floats in this order.
struct SensorPacket {
  float quaternion[4];  // w, x, y, z
  float accel[3];       // m/s^2
  float gyro[3];        // rad/s
  float mag[3];         // microtesla
};

Adafruit_Arcada arcada;

// CLUE boards ship with one of two nearly identical motion chips.
Adafruit_LSM6DS33 lsm6ds33;
Adafruit_LSM6DS3TRC lsm6ds3trc;
Adafruit_LSM6DS *imu;
Adafruit_LIS3MDL magnetometer;

BLEService orientationService(ORIENTATION_SERVICE_UUID);
BLECharacteristic sensorsChar(SENSORS_UUID, CHR_PROPS_READ | CHR_PROPS_NOTIFY, sizeof(SensorPacket), true);

// Rotation from the board's frame (x right, y toward USB, z out of the screen)
// to the world frame, where z is up.
Quaternion orientation;

// Set by the IMU's data-ready interrupt, cleared when loop() consumes the sample.
// The CPU cycle counter gives us a precise timestamp for when the sample was taken.
volatile bool sampleReady = false;
volatile uint32_t sampleCycles = 0;

void onImuDataReady() {
  sampleCycles = DWT->CYCCNT;
  sampleReady = true;
}

void setup() {
  Serial.begin(115200);
  dwt_enable();  // start the CPU cycle counter
  setupDisplay();
  setupImu();
  setupBle();
}

void loop() {
  static uint32_t lastSampleCycles = DWT->CYCCNT;
  static uint32_t lastSendMs = 0, lastShowMs = 0;

  // Wait for the data-ready interrupt. Also go if INT1 is already high: data-ready stays
  // high until we read, so a sample that landed before we attached the interrupt has no edge.
  if (!sampleReady && !digitalRead(IMU_INTERRUPT_PIN)) {
    return;
  }
  sampleReady = false;
  float dt = (float)(sampleCycles - lastSampleCycles) / SystemCoreClock;
  lastSampleCycles = sampleCycles;

  sensors_event_t accel, gyro, temp;
  imu->getEvent(&accel, &gyro, &temp);
  updateOrientation(accel, gyro, dt);

  if (every(1000 / SEND_HZ, lastSendMs)) {
    sendPacket(accel, gyro);
  }
  if (every(1000 / DISPLAY_HZ, lastShowMs)) {
    showStatus();
  }
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

void sendPacket(const sensors_event_t &accel, const sensors_event_t &gyro) {
  sensors_event_t mag;
  magnetometer.getEvent(&mag);

  SensorPacket packet = {
    {orientation.a, orientation.b, orientation.c, orientation.d},
    {accel.acceleration.x, accel.acceleration.y, accel.acceleration.z},
    {gyro.gyro.x, gyro.gyro.y, gyro.gyro.z},
    {mag.magnetic.x, mag.magnetic.y, mag.magnetic.z},
  };
  sensorsChar.notify(&packet, sizeof(packet));
}

// True once every periodMs milliseconds. lastMs remembers when it was last true.
bool every(uint32_t periodMs, uint32_t &lastMs) {
  uint32_t now = millis();
  if (now - lastMs < periodMs) {
    return false;
  }
  lastMs = now;
  return true;
}

void setupDisplay() {
  if (!arcada.arcadaBegin()) {
    halt("Arcada failed");
  }
  arcada.displayBegin();
  arcada.setBacklight(255);
  arcada.display->setTextSize(2);
  arcada.display->setTextColor(ARCADA_WHITE, ARCADA_BLACK);
}

void setupImu() {
  if (lsm6ds33.begin_I2C()) {
    imu = &lsm6ds33;
  } else if (lsm6ds3trc.begin_I2C()) {
    imu = &lsm6ds3trc;
  } else {
    halt("No IMU found");
  }
  if (!magnetometer.begin_I2C()) {
    halt("No magnetometer found");
  }
  Wire.setClock(400000);  // both chips support fast mode I2C; begin_I2C() left it at 100 kHz

  // Sample at 416 Hz and raise INT1 whenever a new gyro reading is ready.
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

void showStatus() {
  arcada.display->setCursor(0, 0);
  arcada.display->println(Bluefruit.connected() ? "BLE connected  " : "BLE advertising");
  arcada.display->println();
  showValue("w", orientation.a);
  showValue("x", orientation.b);
  showValue("y", orientation.c);
  showValue("z", orientation.d);
}

void showValue(const char *label, float value) {
  arcada.display->print(label);
  arcada.display->print(": ");
  arcada.display->print(value, 2);
  arcada.display->println("   ");  // trailing spaces erase leftovers from a longer number
}

void halt(const char *message) {
  Serial.println(message);
  arcada.display->println(message);
  while (true) {
  }
}
