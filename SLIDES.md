# Slide topics: brainstorm

A working list to drill down with the class on Friday: which topics they want, and which
plug-in sensors to order. Each topic has the idea in one line, what to cover, a demo, a
story from building day 1, and a question to put to the class. The pinout, schematic and
every datasheet are in [`datasheets/`](datasheets/README.md).

## 1. Wire protocols: I2C and SPI

**The idea:** chips talk over a few shared wires, and the protocol decides who talks when.

- I2C: two wires (clock SCL, data SDA) shared by every chip. Each chip has an address; the
  CLUE's five sensors are 0x1C, 0x39, 0x44, 0x6A and 0x77. Every byte is acknowledged.
- The lines are open-drain: chips only pull low, and resistors pull high. That is why a
  stuck chip can hold the whole bus, and why the pull-up value sets the top speed.
- Speeds: 100 kHz standard, 400 kHz fast. Rise time limits are 1000 ns and 300 ns.
- SPI: clock, data out, data in, and one chip-select wire per chip. Push-pull, no
  addresses, no acknowledgements, tens of MHz. The CLUE's screen is on SPI; the sensors
  are on I2C.
- Trade-offs: wires per chip, speed, how failures show up, how many chips fit.
- STEMMA QT and Qwiic are just I2C on a 4-pin plug.

**Demo:** the Moku's scope and I2C decoder on SDA and SCL while the board runs. Then rest a
finger on the edge fingers between 3V and GND and watch the edges round off.

**Story:** a finger there made the board reboot "stuck in the IMU". 4.7 kΩ pull-ups times
about 150 pF of bus plus finger gives a rise time near 600 ns, twice the 400 kHz limit.
Dropping to 250 kHz fixed a resting finger; a hard squeeze still bridges the two lines.
Also: the MPR121 and DRV2605L both live at 0x5A, so one has to move.

**Ask:** why would a production design (the TechDisc) put everything on SPI instead?

## 2. Analog in

**The idea:** an ADC turns a voltage into a number by comparing it against a reference.

- Resolution: the nRF52840 ADC is 12 bits (14 with oversampling), so 4096 steps.
- Reference and range: by default 0 to 3.6 V full scale; never more than 3.3 V on a pin.
- Sample rate and Nyquist: sample at least twice the fastest thing you want to see.
- Noise, and averaging it away.
- Only some pins can do it: the AIN pins on the pinout (A0 to A7).
- Most CLUE sensors do their own conversion and send a number over I2C. The microphone
  is not analog either: PDM sends one bit about a million times a second, and the chip
  averages the bits into sound.

**Demo:** a potentiometer on pad 0 plotted live; then the Moku's signal generator into the
same pad at rising frequencies until the samples cannot keep up.

**Ask:** how would you measure the 3×AAA battery, which can be 4.8 V when fresh?
(Answer: a voltage divider, which leads into topic 5.)

## 3. Pin outputs: high, low and high-Z

**The idea:** a pin can drive high, drive low, or let go (high impedance) and just listen.

- Push-pull output drives both ways; open-drain only pulls low (I2C).
- An input is high-Z: it barely touches the wire, so a floating input reads noise.
- Drive strength: a few mA in standard drive, about 10 mA in high drive; check the
  nRF52840 spec before lighting an LED straight off a pin.
- 3.3 V logic only; HS and LS pins on the pinout (LS pins switching above 10 kHz can
  disturb the radio).
- Shorting an output driven high to one driven low is how pins die.

**Demo:** read a floating pin and wave a hand near it. Then the touch pads.

**Story:** our touch sensing is exactly this: drive the pad high, switch it to high-Z, and
count how long it stays high. A finger adds capacitance, so it stays high longer. To free
a stuck I2C bus at startup we drive SCL in open-drain mode and clock it nine times.

**Ask:** what happens if two chips both drive the same wire, one high and one low?

## 4. Pull-ups and pull-downs

**The idea:** a resistor gives a wire a default value when nothing is driving it.

- Buttons: A and B use the chip's internal pull-up (about 13 kΩ), so pressed reads LOW.
- I2C needs pull-ups to work at all: 4.7 kΩ on the CLUE (see the schematic).
- Choosing a value: smaller is faster and stronger but wastes current while low; larger
  saves power but is slow and picks up noise. That is an RC time constant again.
- Every STEMMA QT breakout adds its own pull-ups, so plugging in more boards puts them
  in parallel and lowers the total.
- Pull-down for the opposite default; the touch pads like 1 MΩ to GND.
- Switch bounce, and debouncing.

**Demo:** a button with no pull-up versus with one, read on the screen or the scope.

**Ask:** two 10 kΩ breakouts on the CLUE's 4.7 kΩ: what is the pull-up now? (About 2.4 kΩ.)

## 5. Resistors, voltage and current

**The idea:** V = I × R, and almost everything else follows from it.

- Ohm's law, power P = V × I, and why resistors get hot.
- Series and parallel.
- LED current limiting: from 3.3 V, a red LED drops about 2 V, so 10 mA needs 130 Ω.
  The CLUE's own red LED has a 1 kΩ resistor (about 1.3 mA), see the schematic.
- Voltage dividers: reading the battery, and how sensors like potentiometers work.
- RC time constants, which tie the touch pads, I2C rise times and debouncing together.
- Reading color codes, and checking with the multimeter.

**Demo:** the resistor kit, the multimeter, an LED on pad 0 with the wrong resistor and the
right one.

**Ask:** pick a resistor for an LED at 5 mA, then check it with the meter.

## 6. Bluetooth Low Energy

**The idea:** a device advertises; something connects; then it reads, writes and
subscribes to characteristics.

- Advertising: 31 bytes, a few times a second. Our 128-bit service ID fills most of it, so
  the name rides in the scan response.
- Connections and the connection interval (7.5 ms to 4 s). Windows starts at 15 ms and
  then moves to 60 ms, which caps how many packets a second get through.
- GATT: a service is a folder of characteristics; each characteristic has an ID, a value,
  and what you may do with it (read, write, notify).
- Subscriptions: the phone or page asks to be notified; the board pushes new values.
  Notifications are not acknowledged; indications are.
- MTU: 23 bytes by default, 247 here, which is why the 60-byte motion packet fits.
- When the radio queue is full, the board skips ahead, and the stats row counts misses.
- Supervision timeout: Windows asked for 9.6 s, which is how long it takes the laptop to
  notice a board that reset.
- 2.4 GHz shared with wifi; range; one connection at a time here.

**Demo:** the class page, then nRF Connect or Bluefruit Connect on a phone subscribing to
a single characteristic by hand.

**Story:** asking for a bigger MTU from the board stalled Windows for 30 seconds, so the
laptop asks instead; and Bluefruit's own notify waited up to 100 ms when the queue was
full, which starved the motion fusion.

**Ask:** why is each sensor its own characteristic instead of one big packet?

## Other candidates

These came up while building day 1, or are already planned:

- Quaternions and sensor fusion: device-frame turns on the right, world-frame on the left.
- Calibration: offsets, the sphere fit, and why the magnetometer hates steel desks.
- PCBs, with the TechDisc as the demo.
- Debugging a board you cannot see inside: the watchdog, the stall check, and logging over
  Bluetooth.
- Sound: PDM microphones and decibels.
- Power: batteries, regulators, and a current budget.

## For Friday: the sensor menu

Prices are from the cart as it was built; check them before ordering. Ask each student
what they would build, then order enough that two people can use the popular ones.

| Sensor | What it does | A project | Price |
| --- | --- | --- | --- |
| VL53L1X | Distance, 3 cm to 4 m, by timing light | Theremin, parking sensor, people counter | $14.95 |
| AMG8833 | 8×8 thermal camera | Heat vision on the CLUE's screen | $44.95 |
| MPR121 | 12 capacitive touch inputs | Fruit piano, touch keyboard | $6.95 |
| DRV2605L + motor disc | Haptic buzzes, 100+ built-in effects | Buzzing compass, silent alarm | $7.95 + $1.95 |
| ANO encoder | Dial with five buttons | Menus, a game controller | $13.95 |
| PA1010D | GPS | Track a walk; needs a window or outdoors | $29.95 |
| SCD-43 | True CO2, temperature, humidity | Room air monitor that says "open a window" | $49.95 |
| LTR390 | UV and light | Sunburn meter | $4.95 |
| MLX90632 | Non-contact temperature | Point-and-shoot thermometer | $17.50 |
| Micro servo | Moves something, on pads 0 to 2 | A sensor that points or waves | not in the cart yet |
| NeoPixel strip | Addressable color LEDs on pad 0 | Light bar, level meter | not in the cart yet |

**Ask the class:** what would you build in a week? Which of the topics above sound
interesting? Who has written code before, and in what?
