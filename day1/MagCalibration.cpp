#include "MagCalibration.h"
#include <math.h>

// Only count readings that moved at least this far from the last one, so a board sitting
// still does not pile thousands of identical points onto one spot of the sphere.
const float MIN_SPACING = 5;  // microtesla
const int MIN_POINTS = 50;

// Earth's field is 25 to 65 microtesla. A fit outside this range was not measuring Earth.
const float MIN_RADIUS = 20;
const float MAX_RADIUS = 70;

// Once there is a fit, a reading this far off its sphere (as a fraction of the radius) is
// not Earth's field either, so it is left out rather than allowed to drag the fit around.
const float MAX_OFF_SPHERE = 0.25;

// A direction counts once the gravity direction's dot product with that axis reaches this,
// which is within 60 degrees of pointing that face straight down.
const float MIN_DOT = 0.5;

void MagCalibration::add(const float mag[3], const float accel[3]) {
  // Which axis is down lined up with? Only trust the accelerometer when it is reading close
  // to one g, meaning the board is turning gently rather than being shaken.
  float g = sqrt(accel[0] * accel[0] + accel[1] * accel[1] + accel[2] * accel[2]);
  if (g > 8 && g < 12) {
    for (int i = 0; i < 3; i++) {
      if (accel[i] > MIN_DOT * g) {
        faceSeen[2 * i] = true;
      } else if (accel[i] < -MIN_DOT * g) {
        faceSeen[2 * i + 1] = true;
      }
    }
  }

  if (radius > 0 && !isEarthField(mag)) {
    return;  // a desk full of wires or a magnet nearby would drag the fit off
  }
  if (count > 0 && distance(mag, last) < MIN_SPACING) {
    return;  // the board has not moved; this point is already in the fit
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

bool MagCalibration::isEarthField(const float mag[3]) const {
  return radius > 0 && fabs(distance(mag, offset) - radius) <= MAX_OFF_SPHERE * radius;
}

float MagCalibration::distance(const float a[3], const float b[3]) {
  float dx = a[0] - b[0], dy = a[1] - b[1], dz = a[2] - b[2];
  return sqrt(dx * dx + dy * dy + dz * dz);
}

int MagCalibration::facesCovered() const {
  int covered = 0;
  for (int i = 0; i < 6; i++) {
    covered += faceSeen[i];
  }
  return covered;
}

int MagCalibration::facesSeen() const {
  int bits = 0;
  for (int i = 0; i < 6; i++) {
    bits |= faceSeen[i] << i;
  }
  return bits;
}

bool MagCalibration::solve() {
  if (count < MIN_POINTS || facesCovered() < 6) {
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
    reset();  // the readings came from somewhere magnetic; start over
    return false;
  }
  offset[0] = cx;
  offset[1] = cy;
  offset[2] = cz;
  radius = sqrt(r2);
  return true;
}

// Forgets every reading so far. The offset and radius found so far are kept.
void MagCalibration::reset() {
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      ata[i][j] = 0;
    }
    atb[i] = 0;
  }
  for (int i = 0; i < 6; i++) {
    faceSeen[i] = false;
  }
  count = 0;
}
