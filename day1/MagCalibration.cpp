#include "MagCalibration.h"
#include <math.h>

// Only count readings that moved at least this far from the last one, so a board sitting
// still does not pile thousands of identical points onto one spot of the sphere.
const float MIN_SPACING = 5;  // microtesla
const int MIN_POINTS = 50;

// Earth's field is 25 to 65 microtesla. A fit outside this range means something
// magnetic is swamping the chip.
const float MIN_RADIUS = 20;
const float MAX_RADIUS = 100;

// An octant only counts when down is clearly inside it: every component of the gravity
// direction at least this fraction of the whole, so the board is tipped on a corner.
const float MIN_TILT = 0.3;

void MagCalibration::add(const float mag[3], const float accel[3]) {
  // Which octant is down in? Only trust the accelerometer when it is reading close to
  // one g, meaning the board is turning gently rather than being shaken.
  float g = sqrt(accel[0] * accel[0] + accel[1] * accel[1] + accel[2] * accel[2]);
  if (g > 8 && g < 12 && fabs(accel[0]) > MIN_TILT * g && fabs(accel[1]) > MIN_TILT * g && fabs(accel[2]) > MIN_TILT * g) {
    octantSeen[(accel[0] > 0) | (accel[1] > 0) << 1 | (accel[2] > 0) << 2] = true;
  }

  float dx = mag[0] - last[0], dy = mag[1] - last[1], dz = mag[2] - last[2];
  if (count > 0 && dx * dx + dy * dy + dz * dz < MIN_SPACING * MIN_SPACING) {
    return;
  }
  for (int i = 0; i < 3; i++) {
    last[i] = mag[i];
  }
  count++;

  // A point on a sphere with center c and radius r satisfies
  //   2x*cx + 2y*cy + 2z*cz + (r^2 - |c|^2) = x^2 + y^2 + z^2
  // which is linear in the four unknowns cx, cy, cz and d = r^2 - |c|^2. Each reading
  // is one row of A*u = b; we keep the least-squares normal equations (A'A)u = A'b.
  double row[4] = {2 * mag[0], 2 * mag[1], 2 * mag[2], 1};
  double b = mag[0] * mag[0] + mag[1] * mag[1] + mag[2] * mag[2];
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      ata[i][j] += row[i] * row[j];
    }
    atb[i] += row[i] * b;
  }
}

int MagCalibration::octantsCovered() const {
  int covered = 0;
  for (int i = 0; i < 8; i++) {
    covered += octantSeen[i];
  }
  return covered;
}

bool MagCalibration::solve() {
  if (count < MIN_POINTS || octantsCovered() < 8) {
    return false;
  }

  // Solve the 4x4 system by Gauss-Jordan elimination on the augmented matrix [A'A | A'b].
  double m[4][5];
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      m[i][j] = ata[i][j];
    }
    m[i][4] = atb[i];
  }
  for (int col = 0; col < 4; col++) {
    int pivot = col;
    for (int r = col + 1; r < 4; r++) {
      if (fabs(m[r][col]) > fabs(m[pivot][col])) {
        pivot = r;
      }
    }
    for (int k = 0; k < 5; k++) {
      double t = m[col][k];
      m[col][k] = m[pivot][k];
      m[pivot][k] = t;
    }
    if (m[col][col] == 0) {
      return false;
    }
    for (int r = 0; r < 4; r++) {
      if (r != col) {
        double f = m[r][col] / m[col][col];
        for (int k = col; k < 5; k++) {
          m[r][k] -= f * m[col][k];
        }
      }
    }
  }

  double cx = m[0][4] / m[0][0], cy = m[1][4] / m[1][1], cz = m[2][4] / m[2][2], d = m[3][4] / m[3][3];
  double r2 = d + cx * cx + cy * cy + cz * cz;
  if (r2 < MIN_RADIUS * MIN_RADIUS || r2 > MAX_RADIUS * MAX_RADIUS) {
    return false;
  }
  offset[0] = cx;
  offset[1] = cy;
  offset[2] = cz;
  radius = sqrt(r2);
  return true;
}
