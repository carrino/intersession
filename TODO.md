# TODO

- **One-time offset calibration.** Press A and B together once: measure gyro bias while
  still, fit spheres to accel and mag while pointing each face at the floor, save the three
  per-axis offsets to the board's internal flash, load them at every boot. Replaces the
  startup gyro averaging and the per-boot mag fit.
- **STEMMA QT plug-in sensors.** Probe for optional I2C sensors at startup and publish them.
