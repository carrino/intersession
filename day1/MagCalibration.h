#ifndef MAG_CALIBRATION_H
#define MAG_CALIBRATION_H

// Finds the magnetometer's hard-iron offset by fitting a sphere to its readings.
//
// Earth's field has a constant strength, so as the board turns the readings should
// trace a sphere centered on the origin. Magnetized parts near the chip add a constant
// field in the board's frame, which shifts the whole sphere. Its center is the offset
// to subtract from every reading.
//
// The fit is only trusted once the readings cover the sphere. The accelerometer tells
// which way is down in the board's frame; we wait until down has pointed into all eight
// octants, which happens when each corner of the board has been tipped toward the floor
// with the board face up and again face down.
class MagCalibration {
public:
  // Feed every raw magnetometer reading in microtesla, with the accelerometer reading
  // taken at the same time in m/s^2. Both are x, y, z.
  void add(const float mag[3], const float accel[3]);

  // Fit the sphere to the readings so far. Updates offset and radius and returns true
  // once the readings cover the sphere and the fit looks like Earth's field.
  bool solve();

  int octantsCovered() const;  // 0 to 8

  float offset[3] = {0, 0, 0};
  float radius = 0;
  int count = 0;  // readings used in the fit

private:
  // Running sums for the least-squares fit, see MagCalibration.cpp.
  double ata[4][4] = {};
  double atb[4] = {};
  float last[3] = {0, 0, 0};
  bool octantSeen[8] = {};
};

#endif
