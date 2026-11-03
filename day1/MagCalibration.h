#ifndef MAG_CALIBRATION_H
#define MAG_CALIBRATION_H

// Finds the magnetometer's hard-iron offset by fitting a sphere to its readings.
//
// Earth's field has a constant strength, so as the board turns the readings should
// trace a sphere centered on the origin. Magnetized parts near the chip add a constant
// field in the board's frame, which shifts the whole sphere. Its center is the offset
// to subtract from every reading.
class MagCalibration {
public:
  // Feed every raw reading, in microtesla.
  void add(float x, float y, float z);

  // Fit the sphere to the readings so far. Updates offset and radius and returns true
  // once there are enough readings and the fit looks like Earth's field.
  bool solve();

  float offset[3] = {0, 0, 0};
  float radius = 0;
  int count = 0;  // readings used in the fit

private:
  // Running sums for the least-squares fit, see MagCalibration.cpp.
  double ata[4][4] = {};
  double atb[4] = {};
  float last[3] = {0, 0, 0};
};

#endif
