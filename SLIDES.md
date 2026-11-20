# Slide topics: brainstorm

A working list to drill down with the class on Friday: which topics they want. Each topic
has the idea in one line, what to cover, a demo, often a story from building day 1, and a
question to put to the class. The pinout, schematic and every datasheet are in
[`datasheets/`](datasheets/README.md).

## Lecture plan

Five 45-minute lectures, each three to five short blocks with a demo in each. The minutes
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
5. **The physics inside**
   - Sound (10 min)
   - Electricity and magnetism (10 min)
   - Inductors (8 min)
   - Speakers and microphones (7 min)
   - What each sensor really measures (10 min)

For three lectures, keep 1 and 4 and merge 2 and 3: pull-ups, analog in, and I2C and SPI,
leaving binary, sensor quality and datasheets for when they come up during project time.
The physics blocks each stand alone, so without a fifth slot they can open project days,
one a morning. Quaternions and PCBs are their own sessions; see the end.

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
spare port of any plugged-in sensor, since they pass the bus through: blue is SDA, yellow
SCL, black GND.

- For the decoded bytes, the Logic Analyzer: SDA and SCL on any two of the Moku:Go's 16
  digital pins (they are all the same) and a ground. Digital inputs barely load the bus.
- For the edge shapes, the Oscilloscope's analog inputs. Use 10× probes there if there are
  any (the switch on the probe, with the channel set to match): a 1× probe adds about as
  much capacitance as the finger does.

Trigger on SDA falling. The bus is busy: a 14-byte motion read every 2.4 ms, so zoom into
one to see the address, the register, and the acknowledgements.

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
same. Each fix was to stop waiting. And the watchdog bit back: once started it cannot be
stopped, even by the reset that begins an upload, and the bootloader spends six seconds
erasing the old program without checking in. Set to five seconds, it killed uploads
halfway. It is thirty now.

**Demo:** add `delay(500)` to the loop and watch the page's orientation stutter.

## PWM: dimming, servos and tones

**The idea:** switch a pin on and off fast, and the fraction of time it is on becomes the
signal.

- Duty cycle: on 25% of the time looks like a quarter brightness.
- `analogWrite()` dims an LED; the red LED on the back is free to try it on.
- Servos read the pulse width: 1 to 2 ms every 20 ms sets the angle.
- `tone()` is a square wave at a frequency: button A's beep.

**Demo:** the Moku on a PWM pin while the duty cycle changes, then a servo on pad 0.

## Bluetooth Low Energy

**The idea:** a device advertises; something connects; then it reads, writes and
subscribes to characteristics.

- Advertising: 31 bytes, a few times a second. Our 128-bit service ID fills most of it, so
  the name rides in the scan response.
- Connections and the connection interval (7.5 ms to 4 s). Windows starts at 15 ms and
  then moves to 60 ms, which caps how many packets a second get through, so the board
  asks for 15 ms back. The laptop decides; the board can only ask.
- GATT: a service is a folder of characteristics; each characteristic has an ID, a value,
  and what you may do with it (read, write, notify).
- Subscriptions: the phone or page asks to be notified; the board pushes new values.
  Notifications are not acknowledged; indications are.
- MTU: 23 bytes by default, 247 here, which is why the 60-byte motion packet fits.
- When the radio queue is full, the board skips ahead, and the stats row counts misses.
- Supervision timeout: how long each side waits in silence before calling the link dead.
  Windows picks 9.6 s; the board asks for 2 s.
- Names: a laptop lists a board under the name it learned the last time it connected,
  not the one being advertised now, which is why the page reconnects after a rename.
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

# Lecture 5: The physics inside

Every number on the page starts as something moving, bending or warming up inside a chip.
These blocks say what, and each stands alone.

## Sound

**The idea:** sound is air pressure wobbling, hundreds or thousands of times a second,
and the wobble travels as a wave.

- A speaker cone pushes the air next to it, and that air pushes the air beyond. The air
  itself only shuffles back and forth; the squeeze is what travels, at about 343 m/s.
- Frequency is wobbles per second, in Hz, and is heard as pitch. 440 Hz is the A an
  orchestra tunes to; ears cover about 20 Hz to 20 kHz.
- Wavelength = speed ÷ frequency: 3.4 m at 100 Hz, 34 cm at 1 kHz, 3.4 cm at 10 kHz.
  Bass bends round corners and goes through walls; treble casts shadows.
- Loudness is the size of the wobble, and it is tiny: a shout changes the air pressure by
  about one part in a hundred thousand.
- Decibels: ears hear ratios, so loudness is counted in multiples. Ten times the power is
  +10 dB and sounds about twice as loud; double the pressure is +6 dB. 0 dB is the
  quietest sound a young ear can hear.
- A note is rarely one frequency. A string or a voice makes its pitch plus whole-number
  multiples of it, the harmonics, and the mix is why a violin and a flute playing the same
  note sound different. A spectrum shows the mix.
- The CLUE's pressure sensor and its microphone measure the same thing, air pressure. One
  is slow and exact enough to count the floors of a building; the other is fast and only
  notices changes.

**Demo:** the dot grid on the page. Whistle: one column. Say "ah", then "ee": several
columns, and the shape changes with the vowel. Clap: every column at once. Then a phone
playing a steady 250 Hz tone: carry the board round the room and watch the mic row rise
and fall by several dB between spots a step apart, as the sound off the walls adds to the
sound from the phone and cancels it.

**Story:** the mic first read 51 dB in a silent room. Nearly all of that was rumble below
100 Hz that nobody can hear, so the reading now weighs each pitch the way an ear does
(dBA), as sound level meters do.

**Ask:** thunder arrives three seconds after the flash. How far off was the lightning?
(About a kilometer.)

## Electricity and magnetism

**The idea:** they are two halves of one thing. A current makes a magnetic field, and a
changing magnetic field makes a voltage.

- A current makes a magnetic field around its wire (Oersted, 1820). Wind the wire into a
  coil and the turns add up: an electromagnet.
- A changing magnetic field pushes a voltage into any loop of wire it passes through
  (Faraday, 1831). The faster the change, the bigger the voltage. A steady field does
  nothing at all.
- Put the two together and you have most machines. A motor is a current in a magnetic
  field feeling a force. A generator is the same motor turned by hand. A transformer is
  one coil's changing field felt by another, and a wireless charger is a transformer with
  a gap in it.
- Push it one step further and you have radio: a changing electric field makes a changing
  magnetic field, which makes a changing electric field, and the pair travels off at the
  speed of light. Light, wifi and Bluetooth are all this. At Bluetooth's 2.4 GHz one wave
  is 12.5 cm long, which is what sets the size of the antenna.

**Demo:** the page's mag row reads in microtesla. Lay ten turns of wire over the CLUE and
connect a AAA cell through a 10 Ω resistor: the reading jumps by tens of µT, and jumps the
other way with the cell turned round. Then turn a small DC motor by hand with the
multimeter on its leads: a motor is a generator.

**Story:** the magnetometer reads nonsense on a steel desk, a laptop's magnets pulled a
calibration off by 10 µT, and a rover's motors rule the compass out altogether.

**Ask:** a power cord carries the same current out and back in two wires side by side.
What is the magnetic field a meter away? (Almost nothing: the two cancel.)

## Inductors

**The idea:** a coil stores energy in its magnetic field, so the current through it cannot
change in an instant.

- An inductor is just a coil, often wound on a bit of iron or ferrite. It is measured in
  henries.
- Its rule mirrors the capacitor's. A capacitor resists a change in voltage:
  I = C × dV/dt. An inductor resists a change in current: V = L × dI/dt.
- Stop the current suddenly, by opening a switch, and dI/dt is enormous, so the voltage
  is too. The coil kicks, sometimes to hundreds of volts. That is the spark in a spark
  plug, and it is what the diode next to the CLUE's speaker is there to catch
  (lecture 1).
- Used on purpose, the kick makes a higher voltage from a lower one. A boost converter
  switches a coil many thousands of times a second; that is how one 1.5 V cell runs a
  white LED that needs 3 V.
- With a resistor it has a time constant, L ÷ R, just as a capacitor has R × C.
- Every motor, speaker, relay and buzzer is an inductor, whether you wanted one or not.

**Demo:** a coil or a small motor, a AAA cell, and an LED wired backwards across the coil.
Nothing lights while the cell is connected. Break the circuit and the LED flashes: 1.5 V
cannot light an LED, so the flash came out of the coil. Then the Moku across the LED to
see the spike.

**Ask:** what would happen to the transistor that switches the CLUE's speaker if its diode
were left off?

## Speakers and microphones

**The idea:** a speaker is a motor that moves air, and a microphone is the same thing run
backwards.

- A speaker is a coil glued to a cone, sitting in the gap of a magnet. Current one way
  pushes the cone out; the other way pulls it in. Feed it a current that wobbles 440
  times a second and the air gets a 440 Hz sound.
- Size matters. Bass means moving a lot of air slowly, which a phone or laptop speaker
  cannot do. That is why they sound thin, and why the beat finder cannot count on
  hearing the bass drum.
- The CLUE's speaker is a small one switched fully on and off by a transistor, so
  `tone()` gives it a square wave, not a smooth one, which is why it sounds harsh.
- A piezo buzzer works differently: a slice of crystal that bends when a voltage is put
  across it, and makes a voltage when it is bent. Quartz watches and the click of a gas
  lighter use the same effect.
- A dynamic microphone is a speaker used backwards: sound moves the cone, the coil moves
  in the magnet's field, and Faraday gives a voltage.
- The CLUE's microphone is another kind. A skin of silicon a fraction of a millimeter
  across is one plate of a capacitor. Sound pushes it, the gap changes, the capacitance
  changes, and a chip beside it turns that into a stream of single bits (lecture 2).

**Demo:** plug a small speaker or an earbud into the Moku's scope input and talk into it:
a speaker is a microphone. Then drive the same speaker from the signal generator, sweeping
100 Hz to 8 kHz, with the page's dot grid watching. Where does it get quiet?

**Ask:** noise-cancelling headphones have microphones on the outside. What do they play
into your ears?

## What each sensor really measures

**The idea:** nearly every sensor turns its quantity into something moving or changing
inside the chip, then into a capacitance, a resistance or a current it can count.

Pick the rows the class's projects use.

| Sensor | What happens inside | Try |
| --- | --- | --- |
| Accelerometer | A speck of silicon hangs on springs. When the chip speeds up, the speck lags behind and its gap to a fixed plate changes: a capacitor. It cannot tell gravity from acceleration; sitting still it reads 9.8 m/s² upward, the push of the table | Drop it on a pillow: in free fall it reads zero |
| Gyro | A speck of silicon is kept vibrating. Turn the chip and the vibration is pushed sideways, by the same Coriolis effect that curls hurricanes; the sideways motion is read as a capacitance | Spin on an office chair: one turn a second is 6.28 rad/s |
| Magnetometer | A sliver of material whose resistance changes with the magnetic field through it, three of them at right angles | Bring up a magnet, then a steel spoon; Earth alone is 25 to 65 µT |
| Pressure | A thin skin of silicon over a sealed, empty pocket. Air pressure bows it in, and resistors built into the skin change as it stretches | Lift it from the floor to over your head: about 0.2 hPa |
| Humidity | A film of plastic between two plates soaks up water from the air, which changes the capacitance | Breathe on it |
| Temperature | The voltage across a silicon junction falls about 2 mV for each degree | A fingertip; then notice the board warms itself |
| Light and color | Photodiodes: light knocks electrons loose in silicon, giving a current that follows the brightness. Four of them sit behind red, green, blue and clear filters | Colored paper, lit by the white LEDs (button B) |
| Proximity and gesture | An infrared LED flashes, and more of the flash bounces back the closer something is. Four photodiodes side by side see which way a hand went | White paper, then black, at the same distance: it measures the bounce, not the distance |
| Touch pads | A finger is a conductor. Near the pad it adds capacitance, so the pad takes longer to discharge | A fingertip, then a banana on an alligator clip |
| Distance (VL53L1X) | Times a flash of infrared laser light out and back, counting single photons. Light covers 30 cm in a billionth of a second | A sheet of paper at 10 cm, then at 1 m |
| Thermometer gun, thermal camera | Everything warm glows in infrared, more the hotter it is. The glow warms a tiny absorber, and a stack of junctions between two metals turns the warmth into a voltage | A hand, a cold drink, a window |
| CO2 (SCD-43) | CO2 soaks up one particular color of infrared. The sensor flashes that color into a small chamber; the gas that absorbs it warms and swells with each flash, which makes a sound, and a microphone hears how loud | Breathe near it; then open a window |
| UV (LTR390) | A photodiode that answers to ultraviolet | Sunlight straight, then through a window and through sunglasses |
| GPS | Each satellite broadcasts the time from an atomic clock. The receiver compares when four or more arrive, and a billionth of a second is 30 cm. The clocks tick at a different rate up there, by relativity, and the system corrects for it | Indoors, then by a window, then outside |
| Haptic motor | A motor with a lopsided weight on its shaft | Hold it; then let it walk across the table |
| NeoPixel | LEDs: electrons dropping across a junction give off light, and the material sets the color. Each pixel has a red, a green and a blue one and a chip that dims them by switching fast | A phone camera in slow motion sees the switching |

**Ask:** pick a sensor. What would fool it? (Steel for the compass, shaking for the
accelerometer's idea of down, sunlight for the distance sensor, a lamp for the thermal
camera.)

# Beyond the lectures

Bigger topics that deserve their own session, or that fit into project time:

- Quaternions and sensor fusion: device-frame turns on the right, world-frame on the left.
- PCBs, with the TechDisc as the demo.
- A soldering lab: header pins, then a motor disc onto the DRV2605L.
- Debugging a board you cannot see inside: check power first, divide and conquer, the
  watchdog, the stall check, and logging over Bluetooth. Day 1's own bugs (a charge-only
  cable, a jammed I2C bus) make good case studies.
- Power: battery capacity in mAh, a current budget, and how long a project will run.
- Frequency analysis: what the FFT in `Sound.ino` does, and finding a beat or a pitch.
- Version control: this repository, and why every change is a commit.

# For Friday

The talking points and the parts list for the first meeting are in
[`FRIDAY.md`](FRIDAY.md); the reading list for the break is [`READING.md`](READING.md).
