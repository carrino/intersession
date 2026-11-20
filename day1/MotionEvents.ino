// MotionEvents.ino: taps, double taps, falls and steps, spotted by the motion chip itself.
//
// What it adds: setupMotionEvents() turns on the LSM6DS3TR-C's own detectors, and
// readMotionEvents() adds what they saw to readings.events: taps, double taps, falls and
// steps since power-on.
//
// Why: the chip watches its accelerometer at full speed with its own logic, so it catches
// a 10 ms tap or the first 75 ms of a fall that code checking 30 times a second would miss,
// and it costs the processor nothing. The Adafruit library has no calls for these, so they
// are set up register by register from the datasheet (datasheets/LSM6DS3TR-C.pdf, section
// 9). The older LSM6DS33 on first-revision CLUEs is left alone.

const int IMU_ADDRESS = 0x6A;
const uint8_t IMU_CTRL10_C = 0x19, IMU_WAKE_UP_SRC = 0x1B, IMU_TAP_SRC = 0x1C, IMU_STEP_COUNTER = 0x4B;
const uint8_t IMU_TAP_CFG = 0x58, IMU_TAP_THS_6D = 0x59, IMU_INT_DUR2 = 0x5A, IMU_WAKE_UP_THS = 0x5B;
const uint8_t IMU_FREE_FALL = 0x5D;
const uint8_t IMU_FREE_FALL_SEEN = 0x20, IMU_SINGLE_TAP_SEEN = 0x20, IMU_DOUBLE_TAP_SEEN = 0x10;

// The motion events, in the order of readings.events.
enum MotionEvent { EVENT_TAP, EVENT_DOUBLE_TAP, EVENT_FALL, EVENT_STEP, EVENT_COUNT };
uint16_t lastSteps = 0;          // the chip's own step counter at the last check
bool motionEventsOn = false;     // only the LSM6DS3TR-C's detectors are set up

// Turns on the LSM6DS3TR-C's tap, double-tap, free-fall and step detectors. What they see
// is latched until read, so checking 30 times a second misses nothing.
void setupMotionEvents() {
  writeImuRegister(IMU_TAP_CFG, 0x8F);      // detectors on, taps on all three axes, latched
  writeImuRegister(IMU_TAP_THS_6D, 6);      // a tap is over 6 x (4 g / 32) = 0.75 g
  writeImuRegister(IMU_INT_DUR2, 0x7F);     // a tap is under 58 ms, then 29 ms quiet; a second within 0.54 s is a double
  writeImuRegister(IMU_WAKE_UP_THS, 0x80);  // report double taps as well as single ones
  writeImuRegister(IMU_FREE_FALL, 0xFB);    // a fall is 31 samples (75 ms) all under 0.31 g
  writeImuRegister(IMU_CTRL10_C, 0x14);     // the step counter and the functions it runs on
  lastSteps = readSteps();                  // the chip keeps counting through a reset of ours
  motionEventsOn = true;
}

// Counts what the detectors saw since the last check. Returns whether anything happened.
bool readMotionEvents() {
  if (!motionEventsOn) {
    return false;
  }
  uint8_t sources[2];  // WAKE_UP_SRC then TAP_SRC; reading them clears them
  readImuRegisters(IMU_WAKE_UP_SRC, sources, 2);
  uint16_t steps = readSteps();
  int seen[EVENT_COUNT];
  seen[EVENT_TAP] = (sources[1] & IMU_SINGLE_TAP_SEEN) ? 1 : 0;
  seen[EVENT_DOUBLE_TAP] = (sources[1] & IMU_DOUBLE_TAP_SEEN) ? 1 : 0;
  seen[EVENT_FALL] = (sources[0] & IMU_FREE_FALL_SEEN) ? 1 : 0;
  seen[EVENT_STEP] = (uint16_t)(steps - lastSteps);  // the chip's counter wraps at 65536
  lastSteps = steps;
  bool happened = false;
  for (int i = 0; i < EVENT_COUNT; i++) {
    readings.events[i] += seen[i];
    happened = happened || seen[i] > 0;
  }
  return happened;
}

uint16_t readSteps() {
  uint8_t data[2];
  readImuRegisters(IMU_STEP_COUNTER, data, 2);
  return data[0] | (data[1] << 8);
}

void writeImuRegister(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(IMU_ADDRESS);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}

// Reads len registers in a row; the chip moves to the next register by itself.
void readImuRegisters(uint8_t reg, uint8_t *data, int len) {
  Wire.beginTransmission(IMU_ADDRESS);
  Wire.write(reg);
  Wire.endTransmission(false);  // no stop: the read follows straight on
  Wire.requestFrom(IMU_ADDRESS, len);
  for (int i = 0; i < len; i++) {
    data[i] = Wire.read();
  }
}
