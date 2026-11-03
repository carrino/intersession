# Intersession electronics class

Code for the Adafruit CLUE board. Each day lives in its own folder with a sketch named
after the folder (`day1/day1.ino`), which is what the Arduino tools require.

## One-time setup

1. Install [Arduino IDE 2](https://www.arduino.cc/en/software).
2. In **File > Preferences**, add this to *Additional boards manager URLs*:
   `https://adafruit.github.io/arduino-board-index/package_adafruit_index.json`
3. Still in Preferences, set the **Sketchbook location** to this repository folder. That makes
   the `libraries/` folder here available to every sketch.
4. In **Tools > Board > Boards Manager**, install **Adafruit nRF52**. It is a big download.
5. In **Tools > Manage Libraries**, install each of these and click *Install all* when asked
   about dependencies: `Adafruit Arcada Library`, `Adafruit LSM6DS`, `Adafruit LIS3MDL`,
   `Adafruit SHT31 Library`, `Adafruit APDS9960 Library`, `Adafruit BMP280 Library`.

## Every day

1. **Tools > Board > Adafruit nRF52 Boards > Adafruit CLUE**, and pick the CLUE's port.
2. Open the day's sketch and press Upload.
3. Open the day's web page (for example `day1/orientation.html`) in Chrome or Edge, since
   only those have Web Bluetooth, and connect to your board. Its name, `CLUE-xxxx`, is on
   the board's screen.

If an upload fails, double-click the CLUE's reset button. The screen goes dark, a
`CLUEBOOT` drive appears, and the upload will work. Dragging a `.uf2` file onto that drive
also programs the board without any of the tools above.

## Command line

`arduino-cli` reads the `sketch.yaml` in each day folder, which pins the board package and
every library version, and installs what is missing on the first build:

```bash
arduino-cli compile --profile clue day1
```

```bash
arduino-cli upload --profile clue -p /dev/cu.usbmodem* day1
```
