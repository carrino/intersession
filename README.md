# Intersession electronics class

Code for the Adafruit CLUE board. Each day lives in its own folder with a sketch named
after the folder, for example `day1/day1.ino`, and a web page that shows what the board is
doing.

- [Set up your laptop](#set-up-your-laptop): once
- [Set up a board](#set-up-a-board): once per board
- [Every day](#every-day): upload, connect, save your work
- [When something goes wrong](#when-something-goes-wrong)
- [What is in day 1](day1/README.md): the sketch's tabs, and things to try

The [`datasheets`](datasheets/README.md) folder has the CLUE's schematic and pinout and the
datasheet for every sensor on it or in the parts order.

## Set up your laptop

Once. A Windows or Mac laptop you are allowed to install software on.

**Install the tools**

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
5. Make sure you have Chrome or Edge. The web pages use Web Bluetooth, which only those
   browsers have.

**Get the code**

Your own copy, kept under version control, so every change you make is saved with a note
and can be undone.

6. Make a free account at [github.com](https://github.com) if you do not have one.
7. Open [github.com/carrino/intersession](https://github.com/carrino/intersession) and
   click **Fork**, top right. That makes a copy of the repository under your own account.
8. Install [GitHub Desktop](https://desktop.github.com) and sign in. Choose **File > Clone
   repository**, pick your fork, and note the folder it goes in. If it asks how you plan
   to use the fork, choose **For my own purposes**. From a terminal the same thing is
   `git clone https://github.com/<your-name>/intersession.git`.

**Check it**

9. In the Arduino IDE, open `day1/day1.ino` from the folder you cloned and press **Verify**
   (the check mark). Tools > Board should say **Adafruit CLUE**. If it compiles with no
   errors, the laptop is ready. No board is needed for this.

**Getting the class's updates.** The class repository will change after you fork it. On
your fork's page on GitHub, click **Sync fork**, then **Update branch**. Then in GitHub
Desktop click **Fetch origin** and **Pull origin**. Your own commits are kept.

## Set up a board

Once per board. The board keeps its name and its calibration in its own flash, so they
survive uploads and power cuts.

1. Upload day 1 to the board and connect the page to it: the first four steps of
   [Every day](#every-day). A board that has never been calibrated opens the calibration
   screen on the page.
2. **Name it.** Type a name in the box and press Enter. The board then shows up as
   `CLUE-<name>`, so everyone can find their own. The page drops the link and reconnects
   when you do, which is how your laptop learns the new name. Another laptop that knew the
   board by its old name lists it that way until it connects again; the top line of the
   board's own screen is the name it is really using.
3. **Calibrate it.** Hold the board in the air, an arm's length from the laptop and away
   from the table, with each of its six faces toward the floor in turn. A face goes yellow
   when it is within 10 degrees, fills green while you hold still, and locks in after a
   second and a half. Then set the board down on anything and leave it alone. Click a card
   to redo that step. The holds are in the air because laptops have magnets in them and
   most tables have steel, and both throw the magnetometer off; the set-down step only
   uses the gyro, so the table does not matter there.
4. **Check it.** Turn the board about: the picture on the page should follow it, the two
   arrows should hold still, and the offset in the `mag fit` row should stay near zero.

The **Recalibrate** button on the page starts over, and brings the name box back.

## Every day

1. Plug in the CLUE. Open the day's sketch, such as `day1/day1.ino`, from your clone.
2. Check that Tools > Board says **Adafruit CLUE**, which should be picked for you. Then
   pick the board under **Tools > Port**. On Windows it is a `COM` number; on a Mac it
   starts with `/dev/cu.usbmodem`. The port is the one thing the IDE will not choose for
   you, and "no upload port provided" means it was skipped.
3. Press **Upload**. The board's screen comes back with the sensor readings and its
   Bluetooth name on the top line.
4. Open the day's web page, such as `day1/orientation.html`, in Chrome or Edge. Click
   **Connect** and pick your board's name. The name on the board's screen turns from
   yellow to green.
5. When you are done, click **Disconnect**, or close the page. Only one laptop or phone
   can be connected to a board at a time.
6. Save your work as you go: in GitHub Desktop, write a line saying what you changed and
   why, click **Commit**, and at the end of the day **Push**. Small commits, often.

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

## Other ways to load and build

Dragging a `.uf2` file onto the `CLUEBOOT` drive also programs the board without any of the
tools above, which is a quick way to get a known-good day's firmware onto a board.

For checking a build from a terminal, each day folder has a `sketch.yaml` that pins every
version: `arduino-cli compile --profile clue day1`.
