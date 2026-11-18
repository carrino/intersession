// Orientation.ino: which way the board is facing, fused from its three motion sensors.
//
// What it adds: setupImu() starts the motion chip and the magnetometer, readImu() takes
// each gyro and accelerometer sample (416 a second) and updates the orientation, and
// readMag() takes the magnetometer (10 a second) and steers north. The results go in
// readings.quaternion and readings.vectors.
//
// Why: each sensor alone is not enough. The gyro says how the board is turning, which adds
// up to an orientation that slowly drifts. The accelerometer knows which way is down, but
// only when the board is not being shaken. The magnetometer knows north, but only away
// from magnets and steel. So the gyro's turn is applied in full, and the other two nudge
// the result a little at a time. Quaternion.h has the rotation math; MagCalibration.h
// watches the magnetometer's offset.

// How far toward the accelerometer's idea of "down" we move on each IMU sample, and how
// far toward the magnetometer's idea of "north" on each mag reading. Small is smooth but
// slow to fix gyro drift; large is quick but jittery.
const float ACCEL_CORRECTION = 0.005;  // 416 times a second
const float MAG_CORRECTION = 0.05;     // 10 times a second

// CLUE boards ship with one of two nearly identical motion chips.
Adafruit_LSM6DS33 lsm6ds33;
Adafruit_LSM6DS3TRC lsm6ds3trc;
Adafruit_LSM6DS *imu = NULL;
Adafruit_LIS3MDL lis3mdl;
MagCalibration magCalibration;
int imuSamples = 0;  // fused since the last stats update

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

// Finds whichever motion chip this board has, then samples at 416 Hz and raises INT1
// whenever a new gyro reading is ready.
void setupImu() {
  if (lsm6ds33.begin_I2C()) {
    imu = &lsm6ds33;
  } else if (lsm6ds3trc.begin_I2C()) {
    imu = &lsm6ds3trc;
  }
  if (!lis3mdl.begin_I2C()) {
    warn("LIS3MDL missing");
  }
  if (imu == NULL) {
    halt("No IMU found");
  }
  imu->setGyroDataRate(LSM6DS_RATE_416_HZ);
  imu->setAccelDataRate(LSM6DS_RATE_416_HZ);
  imu->configInt1(false, true, false);  // (temperature, gyro, accel) data-ready on INT1
  if (imu == &lsm6ds3trc) {
    setupMotionEvents();
  }
  pinMode(IMU_INTERRUPT_PIN, INPUT);
  attachInterrupt(IMU_INTERRUPT_PIN, onImuDataReady, RISING);
}

const char *imuName() {
  return imu == &lsm6ds3trc ? "LSM6DS3TR-C" : "LSM6DS33";
}

void onImuDataReady() {
  sampleCycles = DWT->CYCCNT;
  sampleReady = true;
}

// Takes a sample if the chip has one. Data-ready stays high until we read, so also go when
// it is already high: a sample that landed before the interrupt was attached never produces
// a rising edge.
void readImu() {
  static uint32_t lastSampleCycles = DWT->CYCCNT;
  if (!sampleReady && !digitalRead(IMU_INTERRUPT_PIN)) {
    return;
  }
  sampleReady = false;
  imuSamples++;
  float dt = (float)(sampleCycles - lastSampleCycles) / SystemCoreClock;
  lastSampleCycles = sampleCycles;

  sensors_event_t accelEvent, gyroEvent, tempEvent;
  imu->getEvent(&accelEvent, &gyroEvent, &tempEvent);
  float *accel = accelEvent.acceleration.v;
  float *gyro = gyroEvent.gyro.v;
  if (calibrated()) {
    calibrateImu(accel, gyro);
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

// Raw until the board is calibrated. After that the stored offset comes off, the sphere
// fit watches for anything the stored offset missed (its own offset should stay near
// zero), and the reading steers north.
void readMag() {
  sensors_event_t magEvent;
  lis3mdl.getEvent(&magEvent);
  float *mag = magEvent.magnetic.v;
  if (calibrated()) {
    calibrateMag(mag);
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

// How many samples were fused in the last second, and a warning if they stop coming.
void updateImuStats() {
  static bool imuSilent = false;
  readings.stats[0] = imuSamples * SLOW_HZ;
  if ((imuSamples == 0) != imuSilent) {
    imuSilent = imuSamples == 0;
    imuSilent ? warn("No IMU samples") : note("IMU samples back");
  }
  imuSamples = 0;
}
