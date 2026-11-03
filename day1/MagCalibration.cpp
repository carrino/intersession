#include "MagCalibration.h"
#include <math.h>

// Only count readings that moved at least this far from the last one, so a board sitting
// still does not pile thousands of identical points onto one spot of the sphere.
const float MIN_SPACING = 5;  // microtesla
const int MIN_POINTS = 50;

// Earth's field is 25 to 65 microtesla. A fit outside this range means the readings
// have not covered enough of the sphere yet, or something magnetic is swamping the chip.
const float MIN_RADIUS = 20;
const float MAX_RADIUS = 100;

void MagCalibration::add(float x, float y, float z) {
  float dx = x - last[0], dy = y - last[1], dz = z - last[2];
  if (count > 0 && dx * dx + dy * dy + dz * dz < MIN_SPACING * MIN_SPACING) {
    return;
  }
  last[0] = x;
  last[1] = y;
  last[2] = z;
  count++;

  // A point on a sphere with center c and radius r satisfies
  //   2x*cx + 2y*cy + 2z*cz + (r^2 - |c|^2) = x^2 + y^2 + z^2
  // which is linear in the four unknowns cx, cy, cz and d = r^2 - |c|^2. Each reading
  // is one row of A*u = b; we keep the least-squares normal equations (A'A)u = A'b.
  double row[4] = {2 * x, 2 * y, 2 * z, 1};
  double b = x * x + y * y + z * z;
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      ata[i][j] += row[i] * row[j];
    }
    atb[i] += row[i] * b;
  }
}

bool MagCalibration::solve() {
  if (count < MIN_POINTS) {
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
