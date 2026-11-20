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

## Brainstorm projects

- What would you build in a week if it worked? Prompts: a game, something you wear, a
  musical instrument, something that measures the world, something that moves, a gift.
- Use [`PROJECTS.md`](PROJECTS.md) for seeds when the room goes quiet: most ideas there
  need nothing but the CLUE.
- Each student leaves with one or two ideas to think about over the break, sharpened with
  the questions at the end of that file.
- Walk the parts list below. Each student picks a first and second choice. There are one
  to three of each, so tally the picks and plan to share the popular ones.

## Ask about topics

- Run through the four lectures in `SLIDES.md` in a sentence each.
- What do you already know, so we can skip it? What sounds most interesting?
- Interest in the bigger sessions: quaternions, PCBs and the TechDisc, a soldering lab.

## Logistics

- **Laptops:** everyone needs one, Windows or Mac, with a USB-C port. The CLUE's port is
  micro-USB, so the class cables are USB-C to micro-USB; anyone with only USB-A ports gets
  one of the spare USB-A cables.
- **Browser:** the class page needs Chrome or Edge.
- **GitHub:** anyone have an account? Everyone needs one: each student forks the class
  repository and commits their work to their own copy, so the week also teaches version
  control.
- **The week:** hours each day, working alone or in pairs, and a demo at the end: who
  comes to see it?
- **Over the break:** how to reach each other with questions.

## Hand out

- The reading list, [`READING.md`](READING.md), with its short C++ list. The repository is
  public, so a link works.
- The install homework, due before class starts: **Set up your laptop** in the README,
  which ends with forking and cloning the repository and pressing Verify on
  `day1/day1.ino`. If it compiles, the laptop is ready; no board needed.

## The parts list

Prices are from the cart; nothing is ordered yet. The AMG8833 thermal camera is out of
stock at Adafruit and will come from Digi-Key. Each part's name links to its page, with pictures and what it can
do.

**Sensors and inputs**

| Part | What it does | A project | Qty | Price |
| --- | --- | --- | --- | --- |
| [VL53L4CX](https://www.adafruit.com/product/5425) | Distance by timing light: about 4 m indoors off a light wall, less in daylight | Theremin, parking sensor, the rover's eyes | 1 | $14.95 |
| [MPR121](https://www.adafruit.com/product/4830) | 12 capacitive touch inputs, alligator-clip pads | Fruit piano, touch keyboard | 3 | $6.95 |
| [ANO encoder](https://www.adafruit.com/product/6310) | Dial with five buttons | Menus, a game controller | 2 | $13.95 |
| [PA1010D](https://www.adafruit.com/product/4415) | GPS | Track a walk; needs a window or outdoors | 1 | $29.95 |
| [SCD-43](https://www.adafruit.com/product/6512) | True CO2, temperature, humidity | Room air monitor that says "open a window" | 1 | $49.95 |
| [LTR390](https://www.adafruit.com/product/4831) | UV and light | Sunburn meter | 1 | $4.95 |
| [MLX90632](https://www.adafruit.com/product/6403) | Non-contact temperature | Point-and-shoot thermometer | 1 | $17.50 |
| [AMG8833](https://www.adafruit.com/product/3538) | An 8×8 picture of temperature, ten times a second | Thermal camera, a hand tracker, a rover that follows you | 1 | Digi-Key |

**Things that light up or move**

| Part | What it does | A project | Qty | Price |
| --- | --- | --- | --- | --- |
| [DRV2605L](https://www.adafruit.com/product/2305) + [motor disc](https://www.adafruit.com/product/1201) | Haptic buzzes, 100+ built-in effects | Buzzing compass, silent alarm | 2 | $7.95 + $1.95 |
| [NeoPixel ring, 16](https://www.adafruit.com/product/1463) | Round ring of color LEDs | A compass ring that lights north | 2 | $9.95 |
| [NeoPixel ring, 12](https://www.adafruit.com/product/1643) | Smaller ring | A clock, a level | 1 | $8.95 |
| [NeoPixel stick, 8 RGBW](https://www.adafruit.com/product/2868) | A bar with a true white LED in each pixel | A desk lamp you can dim | 2 | $7.95 |
| [NeoDriver](https://www.adafruit.com/product/5766) | Drives a NeoPixel strip over I2C, with 5 V data | A meter-long light strip | 2 | $7.50 |
| [IS31FL3741](https://www.adafruit.com/product/5201) | 13×9 RGB LED matrix | Scrolling text, snake, a sensor graph | 1 | $14.95 |
| [Rover chassis kit](https://www.adafruit.com/product/2939) | Two wheels, two motors and a metal frame | Maze solver, line follower, a robot that follows you | 1 | $24.95 |
| [Motor bonnet](https://www.adafruit.com/product/4280) | Drives the rover's motors over I2C | Goes with the rover | 1 | $22.50 |
| Micro servo | Moves something, on pads 0 to 2 | A sensor that points or waves | not in the cart | |

**Power and supplies:** five [3×AAA holders](https://www.adafruit.com/product/727), a [4×AA holder](https://www.adafruit.com/product/830) for the
rover's motors, three
[1200 mAh LiPo batteries](https://www.adafruit.com/product/258) (charged on a Feather board: the CLUE cannot charge
them), a [5 V 2 A supply](https://www.adafruit.com/product/276) for the LED strips with a barrel jack to screw terminal
adapter, thirteen STEMMA QT cables (50, 200 and 400 mm) and four that end in header pins
for a breadboard, alligator clips, conductive fabric, a flux pen. The cart also has a
free KB2040 (an RP2040 board).

## After the meeting

- Place the order with the quantities from the tally.
- Trim the lectures to what they already know, and note who wants which big session.
