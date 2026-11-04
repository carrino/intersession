# Intersession electronics class

Code for the Adafruit CLUE board. Each day lives in its own folder with a sketch named
after the folder, for example `day1/day1.ino`. Open that file in the Arduino IDE and the
other files in the folder come along with it.

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
5. A board that has never been calibrated shows a calibration screen on the page. Hold the
   board in the air, away from the table, with each of its six faces toward the floor in
   turn: a face goes yellow when it is close, fills green while you hold still, and locks in
   after a second. Then set the board down on anything and leave it alone for a second. The
   page works out each sensor's offset and the board keeps them in its flash, so this is
   once per board. The Recalibrate button on the page starts over. The holds are in the air
   because most tables have steel in them, which throws the magnetometer off; the set-down
   step only uses the gyro, so the table does not matter there.
6. Plug a STEMMA QT sensor into the port on the bottom edge. Within half a second the
   screen's `Plug:` line and the page's plug-ins row name it, and the log says when it was
   plugged in or pulled out. Plug it in with the board off when you can: pushing the
   connector in while a sensor is being read can jam the I2C bus, and the board then spends
   two seconds noticing and rebooting itself. Reading the sensor is the next step and takes
   that sensor's library.
7. The three ring pads P0, P1 and P2 on the bottom edge are touch pads. The screen's
   `Touch:` line and the page's touch row count higher while a finger is on one, or on
   anything conductive clipped to it, such as the conductive fabric on an alligator clip.

## When something goes wrong

- **Tools > Port is greyed out and no `CLUEBOOT` drive appears after a double-click reset:**
  the USB cable is charge-only. The board powers up on it but no data gets through. Swap
  the cable. Keep a couple of known-good cables in the room.
- **Upload fails:** double-click the small reset button on the CLUE. The screen goes dark
  and a `CLUEBOOT` drive appears. Upload again.
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
  fingers between the 3V and GND rings on the bottom edge, so a finger resting there, or
  a STEMMA QT connector going in or out mid-read, can jam it. A strip of electrical or
  Kapton tape over the small fingers, leaving the five big rings free, stops the finger
  case. The last value in the page's stats row counts these reboots since power-on, so
  you can tell a one-off from a pattern.
- **"Adafruit CLUE" is not in the board list:** step 3 of the setup did not finish.
- **A `.h` file is not found:** step 4 did not finish, or the sketch was opened from a copy
  of the `.ino` file on its own instead of from its folder.
- **The web page cannot find the board:** make sure nothing else is connected to it. Only
  one laptop or phone can be connected at a time.

Dragging a `.uf2` file onto the `CLUEBOOT` drive also programs the board without any of the
tools above, which is a quick way to get a known-good day's firmware onto a board.

For checking a build from a terminal, each day folder has a `sketch.yaml` that pins every
version: `arduino-cli compile --profile clue day1`.
