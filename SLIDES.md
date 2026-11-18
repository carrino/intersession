# Slide topics: brainstorm

A working list to drill down with the class on Friday: which topics they want. Each topic
has the idea in one line, what to cover, a demo, often a story from building day 1, and a
question to put to the class. The pinout, schematic and every datasheet are in
[`datasheets/`](datasheets/README.md).

## Lecture plan

Four 45-minute lectures, each three or four short blocks with a demo in each. The minutes
are a starting budget, not a script.

1. **Electricity you can hold**
   - Voltage, current and ground (10 min)
   - Resistors and V = I × R, with an LED (15 min)
   - Pin outputs: high, low and high-Z, and transistors as switches (15 min)
   - Safety (5 min)
2. **Inputs and sensors**
   - Pull-ups and pull-downs (10 min)
   - Analog in (15 min)
   - Sensors: resolution, accuracy and noise (15 min)
   - Questions (5 min)
3. **Chips talking to chips**
   - Binary and hex (10 min)
   - I2C and SPI (20 min)
   - Reading a datasheet (15 min)
4. **Time and radio**
   - How your code runs: the loop, blocking and interrupts (15 min)
   - PWM: dimming, servos and tones (10 min)
   - Bluetooth Low Energy (20 min)

For three lectures, keep 1 and 4 and merge 2 and 3: pull-ups, analog in, and I2C and SPI,
leaving binary, sensor quality and datasheets for when they come up during project time.
Quaternions and PCBs are their own sessions; see the end.

# Lecture 1: Electricity you can hold

## Voltage, current and ground

**The idea:** voltage pushes, current flows, and ground is the zero everything is measured
from.

- Voltage is a difference between two points; a single wire has no voltage on its own.
- Current flows around a loop and only around a loop: no return path, no current.
- Ground is just the agreed zero. Two boards talking need a shared ground, or their
  signals mean nothing to each other; every STEMMA QT cable carries GND for that reason.
- Measuring: voltage across a part, current through it.
- The CLUE's numbers: 3.3 V logic from a regulator that takes 3 to 6 V in, so USB or
  3×AAA both work.

**Demo:** the multimeter on the 3V pad, then on a AAA cell, then on the battery pack.

**Ask:** the Moku's probe has a ground clip. What would it show without it?

## Resistors, voltage and current

**The idea:** V = I × R, and almost everything else follows from it.

- Ohm's law, power P = V × I, and why resistors get hot.
- Series and parallel.
- LED current limiting: from 3.3 V, a red LED drops about 2 V, so 10 mA needs 130 Ω.
  The CLUE's own red LED has a 1 kΩ resistor (about 1.3 mA), see the schematic.
- Voltage dividers: reading a battery, and how a potentiometer works.
- RC time constants, which tie the touch pads, I2C rise times and debouncing together.
- Reading color codes, and checking with the multimeter.

**Demo:** the resistor kit, the multimeter, an LED on pad 0 with the wrong resistor and the
right one.

**Ask:** pick a resistor for an LED at 5 mA, then check it with the meter.

## Pin outputs: high, low and high-Z

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

## Transistors as switches

**The idea:** a pin can only supply a few mA, so for anything bigger it flips a switch.

- A MOSFET: a small voltage on the gate lets a big current through.
- The CLUE does this itself: the screen's backlight and the speaker each hang off a
  BSS138 transistor (see the schematic).
- Motors and speakers are coils, which kick back when switched off; the 1N4148 diode next
  to the CLUE's speaker catches that.
- The vibrating motor disc wants about 60 mA at 3 V, far more than a pin, which is why
  the DRV2605L drives it.

**Ask:** find the speaker's transistor and diode on the schematic.

## Safety

- Never connect 3V straight to GND; the regulator and the battery both object.
- Batteries in the holder the right way round; the holder has a switch, use it.
- LiPo batteries: never puncture, bend or short them, and charge them only with a LiPo
  charger, which the CLUE is not.
- The iron: always back in its stand, hold by the handle, and wash hands after handling
  solder. Lead-free still smokes; let the flux fumes blow away.
- Safety glasses for clipping leads, which fly.

# Lecture 2: Inputs and sensors

## Pull-ups and pull-downs

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

## Analog in

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
(A voltage divider, from lecture 1.)

## Sensors: resolution, accuracy and noise

**The idea:** a sensor reading is a number plus an error, and the error has parts.

- Resolution is the smallest step; accuracy is how close to the truth; noise is how much
  it jitters. A reading can be precise and still wrong.
- Offsets and scale: most error is a constant offset, which calibration takes off.
- Averaging and low-pass filters trade noise for lag.
- Units and sanity checks: the accelerometer should read 9.8 m/s² sitting still, and
  Earth's field is 25 to 65 µT.
- Sample rate: fast enough to catch what you care about, and no faster than you can use.

**Story:** an offset measured at power-on, while the gyro was still settling, made it read
0.63 rad/s sitting perfectly still; the magnetometer reads nonsense on a steel desk; the
calibration screen's six holds find each sensor's offsets once and keep them.

**Ask:** your accelerometer reads 10.1 m/s² sitting still. Offset, scale, or noise? How
would you tell?

# Lecture 3: Chips talking to chips

## Binary and hex

**The idea:** chips speak in bytes, and hex is the shorthand for reading them.

- Bits, bytes, and why hex: one hex digit is four bits.
- I2C addresses are written in hex: the CLUE's sensors are 0x1C, 0x39, 0x44, 0x6A and
  0x77.
- Bitmasks: one byte, many on/off flags. The buttons characteristic is A = 1, B = 2;
  the plug-ins row is one bit per sensor.
- Signed numbers: the accelerometer sends two's-complement 16-bit values, which the
  library scales into m/s².
- Little-endian floats are how our Bluetooth packets carry numbers.

**Ask:** buttons reads 3. Which buttons are down?

## I2C and SPI

**The idea:** chips talk over a few shared wires, and the protocol decides who talks when.

- I2C: two wires (clock SCL, data SDA) shared by every chip. Each chip has an address.
  Every byte is acknowledged.
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

To connect it, plug a STEMMA QT to male header cable into the CLUE's port, or into the
spare port of any plugged-in sensor, since they pass the bus through. Clip the probes to
blue (SDA), yellow (SCL) and black (GND). Use 10× probes if there are any: a 1× probe adds
about as much capacitance as the finger does. Trigger on SDA falling. The bus is busy:
a 14-byte motion read every 2.4 ms, so zoom into one to see the address, the register,
and the acknowledgements.

**Story:** a finger there made the board reboot "stuck in the IMU". 4.7 kΩ pull-ups times
about 150 pF of bus plus finger gives a rise time near 600 ns, twice the 400 kHz limit.
Dropping to 250 kHz fixed a resting finger; a hard squeeze still bridges the two lines.
Also: the MPR121 and DRV2605L both live at 0x5A, so one has to move.

**Ask:** why would a production design (the TechDisc) put everything on SPI instead?

## Reading a datasheet

**The idea:** the datasheet is the chip's manual, and you only need five parts of it.

- The front page: what it is and the headline numbers.
- Absolute maximum ratings: the line you never cross.
- Electrical characteristics: what it really does, typical versus guaranteed.
- The interface: the I2C address and the register map.
- The application circuit: the parts to put around it.

**Demo:** find the LTR-390UV's address (0x53) and its part ID register (0x06, which should
read 0xB2) in its datasheet, then read that register over I2C and match the bits by hand.

**Ask:** each student picks their sensor's datasheet and finds its address, supply
voltage, and current draw.

# Lecture 4: Time and radio

## How your code runs: the loop, blocking and interrupts

**The idea:** one processor does everything in turn, so anything that waits makes
everything else wait.

- What happens on upload: compile, the bootloader (the double-click reset), flash, run.
- `setup()` once, then `loop()` forever.
- `delay()` stops everything; checking `millis()` lets the loop keep going (our sketch's
  `every()` helper).
- Interrupts: the hardware taps the processor on the shoulder. The IMU raises a pin when
  a sample is ready, and the sketch only notes the time.
- The watchdog: a timer that resets the board if the loop stops checking in.

**Story:** drawing text on the screen took tens of milliseconds a line and left time for
only 12 motion samples a second; a 20 ms humidity read and Bluetooth's 100 ms wait did the
same. Each fix was to stop waiting.

**Demo:** add `delay(500)` to the loop and watch the page's orientation stutter.

## PWM: dimming, servos and tones

**The idea:** switch a pin on and off fast, and the fraction of time it is on becomes the
signal.

- Duty cycle: on 25% of the time looks like a quarter brightness.
- `analogWrite()` dims an LED; the CLUE's red LED pulse is exactly this.
- Servos read the pulse width: 1 to 2 ms every 20 ms sets the angle.
- `tone()` is a square wave at a frequency: button A's beep.

**Demo:** the Moku on a PWM pin while the duty cycle changes, then a servo on pad 0.

## Bluetooth Low Energy

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
- The radio mode: 1 Mbit or 2 Mbit is how fast bits go while transmitting, not the
  throughput. Most of the time the radio is quiet between connection events, so a link
  carries a few to a few tens of kB/s. The board asks for 2 Mbit; the laptop decides.
- 2.4 GHz shared with wifi; range; one connection at a time here.

**Demo:** the class page, then nRF Connect or Bluefruit Connect on a phone subscribing to
a single characteristic by hand.

**Story:** asking for a bigger MTU from the board stalled Windows for 30 seconds, so the
laptop asks instead; and Bluefruit's own notify waited up to 100 ms when the queue was
full, which starved the motion fusion.

**Ask:** why is each sensor its own characteristic instead of one big packet?

# Beyond the four lectures

Bigger topics that deserve their own session, or that fit into project time:

- Quaternions and sensor fusion: device-frame turns on the right, world-frame on the left.
- PCBs, with the TechDisc as the demo.
- A soldering lab: header pins, then a motor disc onto the DRV2605L.
- Debugging a board you cannot see inside: check power first, divide and conquer, the
  watchdog, the stall check, and logging over Bluetooth. Day 1's own bugs (a charge-only
  cable, a jammed I2C bus) make good case studies.
- Power: battery capacity in mAh, a current budget, and how long a project will run.
- Sound: PDM microphones, decibels, and frequency analysis.
- Version control: this repository, and why every change is a commit.

# For Friday

The talking points and the parts list for the first meeting are in
[`FRIDAY.md`](FRIDAY.md); the reading list for the break is [`READING.md`](READING.md).
