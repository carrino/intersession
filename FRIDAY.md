# Friday: meeting the class

Talking points for the first meeting. The aim is to leave with three things settled: what
each student wants to build, the sensor order, and every laptop able to build the code
before January.

## Open with the demo

Bring a running CLUE and the laptop. Put the 3D page up, hand the board around, and let
them watch it follow along. Then say what the week is: a CLUE each, a few short lectures,
and most of the time building something of their own.

## Ask about them

- What year are you in, and what classes do you like?
- Have you written code before? In what language, and how much: a class, a project, a
  lot?
- Have you built circuits: a breadboard, soldering, a multimeter?
- How much math so far: trigonometry, vectors, matrices? (This sets how deep the
  quaternion session can go.)
- Have you had circuits in a physics class?

## Ask what they want to build

- What would you build in a week if it worked? Prompts: a game, something you wear, a
  musical instrument, something that measures the world, something that moves, a gift.
- Walk the sensor menu below. Each student picks a first and second choice.
- Tally, then settle quantities so the popular sensors can be shared by two.
- Order soon: holiday shipping is slow, and two of the cable lines in the cart were short
  on stock.

## Ask about topics

- Run through the four lectures in `SLIDES.md` in a sentence each.
- What do you already know, so we can skip it? What sounds most interesting?
- Interest in the bigger sessions: quaternions, PCBs and the TechDisc, a soldering lab.

## Logistics

- **Laptops:** Windows or Mac? Your own, or school-managed? School laptops often block
  installing software and sometimes block Bluetooth, which would stop everything. A
  Chromebook or an iPad cannot run the Arduino IDE.
- **Browser:** the class page needs Chrome or Edge.
- **Phones:** iPhone or Android? Both run nRF Connect and Bluefruit Connect for poking at
  the board over Bluetooth. Chrome on Android can also open the class page; on an iPhone,
  use the apps.
- **GitHub:** anyone have an account? Not needed, but handy for keeping their own code.
- **The week:** hours each day, working alone or in pairs, and a demo at the end: who
  comes to see it?
- **Over the break:** how to reach each other with questions.

## Hand out

- The reading list, [`READING.md`](READING.md). The repository is public, so a link works.
- The install homework, due before winter break: the setup in the README, then open
  `day1/day1.ino` and press Verify. If it compiles, the laptop is ready; no board needed.

## The sensor menu

Prices are from the cart as it was built; check them before ordering.

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

## After the meeting

- Place the order with the quantities from the tally.
- Trim the lectures to what they already know, and note who wants which big session.
