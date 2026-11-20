# Day 1: every sensor, an orientation, and a web page

The CLUE's factory demo, plus an orientation fused from the motion sensors and every
reading sent over Bluetooth to `orientation.html`. Setting up and running it is in the
[main README](../README.md); this page is what is inside.

## The sketch's tabs

`day1.ino` is the whole picture: the pins, the list of readings, `setup()` and `loop()`.
Anything that takes more than one library call has its own tab, starting with a header
that says what it adds and why. Open the ones your project uses:

| Tab | What it is |
|---|---|
| `Orientation` | Which way the board is facing, from the gyro, accelerometer and compass |
| `Calibration` | This board's sensor offsets, kept in its flash |
| `MotionEvents` | Taps, double taps, falls and steps, spotted by the motion chip itself |
| `Sound` | The microphone's loudness and its spectrum |
| `Beat` | Finds the beat in music and flashes the back NeoPixel on it |
| `Touch` | The P0, P1 and P2 ring pads as touch pads |
| `Plugins` | Notices STEMMA QT boards being plugged in |
| `Bluetooth` | How the readings get to the page |
| `Name` | A name for each board |
| `Storage` | Small files in the chip's flash, saved so a power cut cannot spoil them |
| `Screen` | The readings on the board's own screen |
| `Log` | What the board has to say: `note()` and `warn()` |
| `Watchdog` | Gets the board going again by itself when the loop stops |

## Things to try once it is running

- **Turn the board.** The page's picture follows it, and the arrows for down and for
  magnetic north hold still.
- **Plug in a STEMMA QT board.** Within half a second the screen's `Plug-in:` line and the
  page's plug-ins row name it. Plug it in with the board off when you can: pushing the
  connector in while a sensor is being read can jam the I2C bus, and the board then spends
  two seconds noticing and rebooting itself. Reading the new sensor takes its own library.
- **Tap it, tap it twice, walk with it, drop it on a pillow.** The page counts taps,
  double taps, steps and falls and says how long ago the last one was. The motion chip
  spots these itself; it needs the LSM6DS3TR-C of the CLUE's 2025 revision, and the
  board's "Started with the ..." log line says which chip it has.
- **Touch the ring pads** P0, P1 and P2 on the bottom edge. The `Touch:` line and the
  page's touch row count higher while a finger is on one, or on anything conductive
  clipped to it.
- **Make a noise.** The mic row is the loudness in dBA, and the grid of dots under the
  table is the sound split into 13 bands from bass to treble.
- **Play dance music.** After a few seconds the NeoPixel on the back flashes on the beat,
  a new color each time, and the beat row shows the tempo. Music with soft or loose drums
  mostly leaves it dark, and a fast song may flash on every other beat.
- **Lift it.** The pressure row turns into feet above sea level, and a lift from the
  floor to over your head shows.
- **Pick a color** for the NeoPixel on the back with the page's Back LED box, and hold
  button B to light the white LEDs for the color sensor.
