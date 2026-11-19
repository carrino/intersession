# Intersession electronics class

Code for the Adafruit CLUE board. Each day lives in its own folder with a sketch named
after the folder, for example `day1/day1.ino`. Open that file in the Arduino IDE and the
other files in the folder come along with it, as tabs across the top.

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

The [`datasheets`](datasheets/README.md) folder has the CLUE's schematic and pinout and the
datasheet for every sensor on it or in the parts order.

## One-time setup

1. Install [Arduino IDE 2](https://www.arduino.cc/en/software).
2. Open **File > Preferences** (Arduino IDE > Settings on a Mac). In *Additional boards
   manager URLs* paste:
   `https://adafruit.github.io/arduino-board-index/package_adafruit_index.json`
3. Open **Tools > Board > Boards Manager**, search for `Adafruit nRF52`, and click Install.
   It is a big download; wait for it to finish.
4. Open **Tools > Manage Libraries**, search for `Adafruit Arcada Library`, and click
   Install. When it asks about dependencies, choose **Install all**. That one install brings
   in every library the CLUE sketches use. It downloads about fifty libraries, so give it a
   few minutes; if the IDE is still showing "Processing" after the last one, quit and reopen
   it. Everything is already installed at that point.

You should now see **Adafruit CLUE** under Tools > Board > Adafruit nRF52 Boards.

## Every day

1. Plug in the CLUE. Open the day's sketch, such as `day1/day1.ino`.
2. Check that Tools > Board says **Adafruit CLUE**, which should be picked for you. Then
   pick the board under **Tools > Port**. On Windows it is a `COM` number; on a Mac it
   starts with `/dev/cu.usbmodem`. The port is the one thing the IDE will not choose for
   you, and "no upload port provided" means it was skipped.
3. Press Upload. The board's screen comes back with the sensor readings and its Bluetooth
   name, `CLUE-xxxx`.
4. Open the day's web page, for example `day1/orientation.html`, in Chrome or Edge. Only
   those browsers have Web Bluetooth. Click Connect and pick your board's name.
5. A board that has never been calibrated shows a calibration screen on the page. First
   type a name in its box and press Enter: the board then shows up as `CLUE-<name>`, so
   everyone can find their own. The laptop that did the naming may go on listing the old
   name for a while, because a laptop remembers each board's name from the last time it
   connected; the top line of the board's own screen is the name it is really using. Then hold the board in the air, away from the table, with
   each of its six faces toward the floor in turn: a face goes yellow when it is within 10
   degrees, fills green while you hold still, and locks in after a second and a half. Then
   set the board down on anything and leave it alone. Click a card to redo that step. The
   page works out each sensor's offset and the board keeps them and its name in its flash,
   so this is once per board. The Recalibrate button on the page starts over. The holds are
   in the air because most tables have steel in them, which throws the magnetometer off;
   the set-down step only uses the gyro, so the table does not matter there.
6. Plug a STEMMA QT sensor into the port on the bottom edge. Within half a second the
   screen's `Plug-in:` line and the page's plug-ins row name it, and the log says when it was
   plugged in or pulled out. Plug it in with the board off when you can: pushing the
   connector in while a sensor is being read can jam the I2C bus, and the board then spends
   two seconds noticing and rebooting itself. Reading the sensor is the next step and takes
   that sensor's library.
7. Tap the board, tap it twice, or walk with it: the page's tap, double tap and steps rows
   count them and say how long ago the last one was. Drop it onto something soft and the
   fall row counts that. The motion chip spots these itself; it needs the LSM6DS3TR-C of
   the CLUE's 2025 revision, and the board's "Started with the ..." log line says which
   chip it has.
8. Play music with a strong, steady beat near the board: dance music works best. After a
   few seconds the NeoPixel on the back flashes on the beat, a new color each time, and the
   page's beat row shows the tempo. Music with soft or loose drums mostly leaves it dark.
   Songs often fit half or double their tempo as well, so a fast song may flash on every
   other beat. It stops within a few seconds of the music stopping.
9. The three ring pads P0, P1 and P2 on the bottom edge are touch pads. The screen's
   `Touch:` line and the page's touch row count higher while a finger is on one, or on
   anything conductive clipped to it, such as the conductive fabric on an alligator clip.

## When something goes wrong

- **Tools > Port is greyed out and no `CLUEBOOT` drive appears after a double-click reset:**
  the USB cable is charge-only. The board powers up on it but no data gets through. Swap
  the cable. Keep a couple of known-good cables in the room.
- **Upload fails:** double-click the small reset button on the CLUE. The screen goes dark
  and a `CLUEBOOT` drive appears. Upload again.
- **The `COM` port changes, say from COM3 to COM4, and the upload cannot find the board:**
  Windows numbers the board differently while it runs a sketch and while it is in its
  bootloader, the mode an upload switches it into first and the one a double-click reset
  leaves it in. It also gives each board its own number. Open Tools > Port again, pick the
  number listed now, and upload again.
- **Upload succeeds but the sketch never starts:** Tools > SoftDevice must match the board.
  Open `INFO_UF2.TXT` on the `CLUEBOOT` drive to see which version the board has.
- **Black screen with the front white LEDs stuck on after an upload, and resets or
  re-uploads do not help:** a sensor is holding the I2C bus. An upload resets the chip at a
  random moment, and if that lands in the middle of a sensor read, the sensor keeps holding
  the data line until something clocks it out. Resets do not clear it because the sensors
  keep their power. **Unplug the USB cable for ten seconds** and plug it back in. The
  current sketch clears the bus by hand at startup and reboots itself within a couple of
  seconds if the loop ever stops, logging what it was doing, so this only bites old builds.
- **The screen says `Stall reboot N: was stuck in the IMU` in red:** a sensor read never
  finished and the board rebooted itself. The I2C bus runs out to the two small gold
  fingers between the 3V and GND rings on the bottom edge, where a finger adds
  capacitance to it. The sketch runs the bus at 250 kHz, slow enough that a finger there
  no longer jams it; at 400 kHz it did. A STEMMA QT connector going in or out mid-read
  can still jam it, and if a board keeps doing this anyway, a strip of Kapton tape over
  the small fingers, leaving the five big rings free, takes fingers out of it. The last
  value in the page's stats row counts these reboots since power-on, so you can tell a
  one-off from a pattern.
- **The screen says `Rebooted by the watchdog` in red:** the loop stopped for thirty
  seconds and even the stall check above did not get to report where. That points at a
  crash or something holding the whole processor, not a stuck sensor. The line also shows
  on the page when it next connects. Note what the board was doing.
- **"Adafruit CLUE" is not in the board list:** step 3 of the setup did not finish.
- **A `.h` file is not found:** step 4 did not finish, or the sketch was opened from a copy
  of the `.ino` file on its own instead of from its folder.
- **The web page cannot find the board:** make sure nothing else is connected to it. Only
  one laptop or phone can be connected at a time.

Dragging a `.uf2` file onto the `CLUEBOOT` drive also programs the board without any of the
tools above, which is a quick way to get a known-good day's firmware onto a board.

For checking a build from a terminal, each day folder has a `sketch.yaml` that pins every
version: `arduino-cli compile --profile clue day1`.
