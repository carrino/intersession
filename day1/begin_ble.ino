// Day 1: read the CLUE's motion sensor, fuse it into an orientation, and
// stream that orientation over Bluetooth LE to orientation.html.
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

// How far toward the accelerometer's idea of "down" we move on each update.
// Small is smooth but slow to fix gyro drift; large is quick but jittery.
const float ACCEL_CORRECTION = 0.02;

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

void setup() {
  Serial.begin(115200);
  setupDisplay();
  setupImu();
  setupBle();
}

void loop() {
  static uint32_t lastMs = millis();
  static int ticks = 0;
  uint32_t now = millis();
  float dt = (now - lastMs) / 1000.0;
  lastMs = now;

  sensors_event_t accel, gyro, temp, mag;
  imu->getEvent(&accel, &gyro, &temp);
  magnetometer.getEvent(&mag);
  updateOrientation(accel, gyro, dt);

  SensorPacket packet = {
    {orientation.a, orientation.b, orientation.c, orientation.d},
    {accel.acceleration.x, accel.acceleration.y, accel.acceleration.z},
    {gyro.gyro.x, gyro.gyro.y, gyro.gyro.z},
    {mag.magnetic.x, mag.magnetic.y, mag.magnetic.z},
  };
  sensorsChar.notify(&packet, sizeof(packet));

  if (++ticks % 10 == 0) {  // the screen is slow, so redraw it less often
    showStatus();
  }
  delay(20);
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
}

void setupBle() {
  Bluefruit.configPrphBandwidth(BANDWIDTH_MAX);  // allow a big enough MTU for the whole packet
  Bluefruit.begin();

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
