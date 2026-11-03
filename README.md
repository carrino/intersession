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
  current sketch also clears the bus by hand at startup, so this only bites old builds.
- **"Adafruit CLUE" is not in the board list:** step 3 of the setup did not finish.
- **A `.h` file is not found:** step 4 did not finish, or the sketch was opened from a copy
  of the `.ino` file on its own instead of from its folder.
- **The web page cannot find the board:** make sure nothing else is connected to it. Only
  one laptop or phone can be connected at a time.

Dragging a `.uf2` file onto the `CLUEBOOT` drive also programs the board without any of the
tools above, which is a quick way to get a known-good day's firmware onto a board.

For checking a build from a terminal, each day folder has a `sketch.yaml` that pins every
version: `arduino-cli compile --profile clue day1`.
