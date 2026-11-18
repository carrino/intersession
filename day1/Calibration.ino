// Calibration.ino: this board's sensor offsets, kept in its flash.
//
// What it adds: the offsets the page's calibration screen works out are stored in a flash
// file, loaded at startup, and taken off every accelerometer, gyro and magnetometer
// reading. The page reads and writes them through the calibration characteristic.
//
// Why: every chip reads a little off zero, by an amount that differs from board to board
// but stays put. Measuring it once (hold each face of the board toward the floor, still,
// then set it down) and subtracting it from then on is most of what makes the orientation
// hold. An unwritten board has all zeros, which means "not calibrated": the readings are
// used raw, the north correction stays off, and the page offers the calibration screen.

struct Calibration {
  float accelOffset[3];
  float gyroOffset[3];
  float magOffset[3];
};
Calibration calibration;
const char *CALIBRATION_FILE = "/calibration.bin";

void loadCalibration() {
  loadFile(CALIBRATION_FILE, &calibration, sizeof(calibration));
}

bool calibrated() {
  static const Calibration none = {};
  return memcmp(&calibration, &none, sizeof(calibration)) != 0;
}

// Each reading becomes raw - offset.
void removeOffset(float reading[3], const float offset[3]) {
  for (int i = 0; i < 3; i++) {
    reading[i] -= offset[i];
  }
}

void calibrateImu(float accel[3], float gyro[3]) {
  removeOffset(accel, calibration.accelOffset);
  removeOffset(gyro, calibration.gyroOffset);
}

void calibrateMag(float mag[3]) {
  removeOffset(mag, calibration.magOffset);
}

void beginCalibrationChar() {
  calibrationChar.setProperties(CHR_PROPS_READ | CHR_PROPS_WRITE);
  calibrationChar.setFixedLen(sizeof(calibration));
  calibrationChar.setWriteCallback(onCalibrationWrite);
  calibrationChar.begin();
  calibrationChar.write(&calibration, sizeof(calibration));
}

// Runs when the page writes a Calibration. All zeros means "forget it and start over".
void onCalibrationWrite(uint16_t connHandle, BLECharacteristic *chr, uint8_t *data, uint16_t len) {
  memcpy(&calibration, data, sizeof(calibration));
  if (calibrated()) {
    bool saved = saveFile(CALIBRATION_FILE, &calibration, sizeof(calibration));
    note(saved ? "Calibration saved" : "ERROR: Calibration not saved");
  } else {
    InternalFS.remove(CALIBRATION_FILE);
    note("Calibration cleared");
  }
}
