# Project ideas: brainstorm

Ideas to choose from for the week. A good week-long project has a small core that works by
the middle of the week, a demo anyone can try at the end, and stretch goals for the time
in between. Most of these work with only what is on the [CLUE][clue]; the rest need one
more part. Each part's name links to its page, with pictures and what it can do, and the
whole list with quantities is in [`FRIDAY.md`](FRIDAY.md). Some projects build on a tab of
day 1's sketch that already does the hard part, named where they do.

## With just the CLUE

| Project | What it does | Uses | Stretch goal |
| --- | --- | --- | --- |
| Bubble level | A bubble on the screen that slides as the board tilts | Accelerometer, screen | Beep when level |
| Magic 8-ball or dice | Shake for an answer or a roll | Accelerometer, screen, speaker | Two boards roll against each other over Bluetooth |
| Compass | Heading on the screen, even tilted | Magnetometer, the day 1 fusion | Point to a saved spot |
| Drop timer | Measures free fall and works out the height from h = ½gt² | Accelerometer | Throw it straight up and measure the peak |
| Pendulum lab | Times a swing and works out g | Gyro | Compare string lengths on a plot |
| Spin meter | Spin rate of anything it is taped to, like a thrown disc | Gyro | The TechDisc, side by side |
| Floor counter | Counts floors climbed from air pressure (about 0.12 hPa per meter) | Pressure sensor | Log a whole day |
| Weather station | Temperature, humidity and pressure logged to flash and plotted on the page | Environment sensors, Bluetooth | A forecast arrow from the pressure trend |
| Color matcher | Hold an object to the sensor; the back LED copies its color | Color sensor, white LEDs, NeoPixel | Name the color on the screen |
| Sound meter | Room loudness lights the NeoPixel green, yellow or red | Microphone | Clap detector; a tuner with frequency analysis |
| Touch piano | Three pads, three notes | Touch pads, speaker | Alligator clips to fruit or foil |
| Gesture clicker | Swipe over the board to change slides on a laptop, as a Bluetooth keyboard | Gesture sensor, Bluetooth | Tilt to point |
| Marble maze | Tilt the CLUE to roll a marble through a maze on a web page, like the wooden box with two knobs | Day 1's orientation, a browser game | Holes to fall in, a timer and levels; the whole game on the CLUE's own screen, no laptop; a buzz from the haptic motor on each wall hit |
| Reaction game | Buttons and screen: who presses first | Buttons, screen, speaker | Leaderboard on the page |
| Step counter | The motion chip counts steps itself; write your own detector and compare | Accelerometer | Steps per minute, distance |
| Tap commands | Tap, double tap and drop do different things | The motion chip's tap and fall detectors | A secret knock |
| Guitar tuner | Names the string you pluck and shows sharp or flat | Microphone: the raw samples from `Sound.ino`, not its spectrum, whose 62.5 Hz steps are far too coarse. Find how long the wave takes to repeat | Any note, sung or whistled; whistle high or low to steer a game on the page |
| Clap-along coach | Shows the tempo of music or of your clapping, and whether you are rushing | Microphone, the beat finder in `Beat.ino` | A metronome you set by tapping the board |
| Air drums | Strap the board to a stick, swing it, and the laptop plays the drum. Start by tapping a pillow, which the tap counter already hears | Gyro, a web page that makes the sound | Different drums for different directions; the stick and a drum kit drawn in 3D on the page, like day 1's board; a second stick |
| Posture coach | Clipped to a shirt, it beeps after ten seconds of slouching | Day 1's orientation, speaker | A day's score on the page |
| Elevator profiler | Rides an elevator and plots its speed and height | Pressure sensor, accelerometer | Compare the two sensors' answers; find the fastest elevator |
| Door counter | Counts people passing the board | Proximity sensor | Tell in from out with the gesture sensor |
| Desk guard | Sounds off and tells your phone when someone moves it or opens the drawer it is in | Accelerometer, light sensor, Bluetooth | Arm and disarm with a secret knock |
| Black box | Records a trip in a backpack, a mailed box or a paper rocket to flash; replay it on the page | Accelerometer, pressure sensor, `Storage.ino` | Find the roughest moment |
| Egg and spoon race | Walk a course holding the board level; tilts and jolts cost points | Day 1's orientation, the step counter | Live leaderboard for the whole class |

## With one part from the order

| Project | What it does | Part | Stretch goal |
| --- | --- | --- | --- |
| Theremin | Hand distance sets the pitch | [VL53L4CX][vl53l4cx] | Second hand controls volume through tilt |
| Parking sensor | Beeps faster as something gets closer | [VL53L4CX][vl53l4cx] | Distance on the screen |
| Fruit piano | Twelve keys made of bananas, foil or pencil drawings | [MPR121][mpr121] | Record and play back |
| Buzzing compass | Buzzes when facing north; wear it | [DRV2605L][drv2605l] and [motor][motor-disc] | Buzz toward a saved spot |
| Silent pager | A phone writes a characteristic and the board buzzes | [DRV2605L][drv2605l] and [motor][motor-disc] | Patterns that mean different things |
| Air monitor | CO2 level as green, yellow or red: "open a window" | [SCD-43][scd43] | A week's log of the classroom |
| Walk tracker | Logs a walk to flash and draws it on a map | [PA1010D][gps] | Speed and distance |
| Sunburn meter | UV index and how long until a burn | [LTR390][ltr390] | Outdoor log |
| Thermometer gun | Point and read a surface temperature | [MLX90632][mlx90632] | Find the coldest window |
| Menu and drawing toy | A dial-driven menu, or an etch-a-sketch on the screen | [ANO encoder][ano] | A small game |
| Compass ring | The LED pointing north stays lit as the board turns | [NeoPixel ring][ring], day 1's orientation | Point to a saved spot |
| Level ring | The ring lights downhill, brighter the steeper the tilt | [NeoPixel ring][ring] | A two-axis level on the 12 and 16 rings |
| Desk lamp | True white you dim with the touch pads | [NeoPixel stick (RGBW)][stick] | Warm to cool by mixing in color |
| Light strip | A meter of NeoPixels that reacts to sound or motion | [NeoDriver][neodriver], a strip, the [5 V supply][supply] | Music visualizer from the microphone |
| Scrolling sign | Text or a live sensor graph on a 13×9 color matrix | [IS31FL3741][matrix] | Snake, steered with the ANO dial |
| Spectrum display | The page's dot grid on real LEDs: the board already makes 13 bands and the matrix is 13 wide | [IS31FL3741][matrix] | Flash the whole matrix on the beat |
| Beat you can feel | The motor taps out the beat of music playing in the room | [DRV2605L][drv2605l] and [motor][motor-disc], the beat finder | A wristband on a battery |
| Catch the light | One LED races round the ring; press the button when it reaches the top | [NeoPixel ring][ring] | Gets faster each round; two players |
| Timer ring | The ring empties as time runs down, then flashes | [NeoPixel ring][ring] | Set the time with a tilt or the touch pads |
| Speed trap | Clocks a toy car or a rolling ball from how fast the distance changes | [VL53L4CX][vl53l4cx] | Chart the run and find the acceleration |
| Water level | Foil strips on the outside of a cup sense the water through it | [MPR121][mpr121] | Fill-me reminder for a plant |
| Plant in a jar | Watch a plant take in CO2 in the light and give it back in the dark | [SCD-43][scd43] | Compare two plants; the same with a person breathing |
| Sunscreen tester | How much UV gets through sunglasses, a window, or sunscreen on cling film | [LTR390][ltr390] | Rank brands; SPF from the numbers |
| Cooling curve | Logs a hot drink cooling without touching it and predicts when it is drinkable | [MLX90632][mlx90632] | Does a lid, a spoon or blowing help most? |
| Satellite clock | The time, to the second, from GPS satellites | [PA1010D][gps] | Speed and heading for a bike |
| Treasure hunt | An arrow points toward a hidden spot outdoors and says how far | [PA1010D][gps], [NeoPixel ring][ring] or the screen | Hide one for another team |

## With the rover

A two-wheeled chassis with the CLUE riding on top. It needs the [chassis kit][chassis],
the [motor bonnet][bonnet] to drive its two motors, and a [4×AA holder][aa-holder] to
power them. There is one rover. It has no wheel sensors, so it cannot tell how far it has
gone; the CLUE's gyro tells it which way it is facing, and that is what all of these lean
on. The motors have magnets in them, so the compass will not be trusted here: heading
comes from the gyro alone.

| Project | What it does | Uses | Stretch goal |
| --- | --- | --- | --- |
| Drive a square | Straight lines and exact quarter turns, ending where it started | The gyro's heading, the motors | A triangle, a star, your initials |
| Obstacle avoider | Drives until something is close, turns away, carries on | [Distance sensor][vl53l4cx] facing forward | Pick the clearer side by looking both ways first |
| Maze solver | Finds its way out of a cardboard maze by keeping one hand on the wall | [Distance sensor][vl53l4cx], the gyro's heading | Draw the maze on the page as it explores; remember the way and run it again faster |
| Line follower | Follows a dark line on the floor | The color sensor facing down, lit by the white LEDs | A fork in the line; a faster lap |
| Remote control | Tilt a second board, or a phone, to steer | Bluetooth, day 1's orientation | Drive it out of sight using only what its sensors report |
| Light seeker | Turns toward the brightest spot and drives to it | Light sensor | Hide from the light instead |
| Dancing robot | Moves on the beat of the music | The beat finder in `Beat.ino` | A different move for each bar |

## With the thermal camera

An [AMG8833][amg8833]: an 8×8 picture of temperature, ten times a second, from 0 to
80 °C. Eight by eight is coarse: a person across the room is a few warm squares. It reads
to a quarter of a degree, but only within about 2.5 °C of the true temperature, so it
shows what is warmer than what, not a fever.

| Project | What it does | Uses | Stretch goal |
| --- | --- | --- | --- |
| Thermal camera | A live heat picture on the page, smoothed up from the 64 squares | [AMG8833][amg8833], Bluetooth | The picture on the CLUE's own screen, to carry around; a freeze button |
| Heat detective | Find the handprint left on a table, the chair someone just left, the draughty window, the charger that is working hardest | [AMG8833][amg8833] | Time how long a handprint lasts on wood, metal and plastic |
| Hand tracker | Finds the warm spot that is your hand and uses where it is as a controller | [AMG8833][amg8833] | Steer the marble maze or play the theremin with no board in your hand |
| People counter | Counts people through a doorway and which way they went, with a camera that cannot recognize anyone | [AMG8833][amg8833] | Keep a running count of who is in the room |
| Heat on LEDs | The picture on the 13×9 color matrix | [AMG8833][amg8833], [IS31FL3741][matrix] | Blue to red through the whole rainbow |
| Follow me | The rover turns toward the warmest thing it sees and drives after it | [AMG8833][amg8833], [the rover](#with-the-rover) | Keep a set distance; lose interest in radiators |
| Heat panorama | Sweep the board round and stitch the frames into one wide picture, placed by which way it was facing | [AMG8833][amg8833], day 1's orientation | The whole room, floor to ceiling |

## Two boards together

| Project | What it does | Uses | Stretch goal |
| --- | --- | --- | --- |
| Light telegraph | One board blinks its LEDs; another reads them with its color sensor and shows the message | White LEDs or NeoPixel, color sensor | Faster codes; colors as extra bits; error checking |
| Remote hands | Tilt one board and the other mirrors it: its screen, its ring, its motor | Bluetooth between boards, day 1's orientation | Steer something on the second board |
| Hot and cold | One board is hidden; the other beeps faster as it gets closer, by radio signal strength | Bluetooth between boards | Three hiders, one seeker |

Two boards talking to each other over Bluetooth is new code: one of them has to play the
laptop's part. Leave a day for it.

## A reach

These might not work, which is part of the point. Try the core on day one before
committing.

| Project | What it does | Uses | The hard part |
| --- | --- | --- | --- |
| Pulse reader | A fingertip on the color sensor, lit by the white LEDs, shows a heartbeat | Color sensor, white LEDs | The pulse is a ripple of about 1% on a signal that moves when the finger does |
| Room mapper | Turn the board on the spot and the page draws the walls around it | [VL53L4CX][vl53l4cx], day 1's orientation | Lining up each distance with the heading it was taken at. Indoors the sensor reaches about 4 m off a light wall and 2 m off a dark one, less in daylight, and its beam is 18° wide: enough to reach a classroom's walls, with the corners coming out soft |
| Vowel spotter | Tells "ee" from "oo" from "ah" by the shape of the spectrum | Microphone, the spectrum | Voices differ; it needs training on each speaker |
| A better beat finder | Make `Beat.ino` follow rock, jazz or a song with no drums | Microphone, the beat finder | Soft onsets are hard to spot, and real players speed up and slow down |

## Questions to sharpen an idea

- What is the one thing it must do on demo day?
- What can you show working by Wednesday?
- Who is it for, and what would they do with it?
- What would you add with a spare day?

[clue]: https://www.adafruit.com/product/4500
[vl53l4cx]: https://www.adafruit.com/product/5425
[mpr121]: https://www.adafruit.com/product/4830
[drv2605l]: https://www.adafruit.com/product/2305
[motor-disc]: https://www.adafruit.com/product/1201
[scd43]: https://www.adafruit.com/product/6512
[gps]: https://www.adafruit.com/product/4415
[ltr390]: https://www.adafruit.com/product/4831
[mlx90632]: https://www.adafruit.com/product/6403
[ano]: https://www.adafruit.com/product/6310
[ring]: https://www.adafruit.com/product/1463
[stick]: https://www.adafruit.com/product/2868
[neodriver]: https://www.adafruit.com/product/5766
[supply]: https://www.adafruit.com/product/276
[matrix]: https://www.adafruit.com/product/5201
[amg8833]: https://www.adafruit.com/product/3538
[chassis]: https://www.adafruit.com/product/2939
[bonnet]: https://www.adafruit.com/product/4280
[aa-holder]: https://www.adafruit.com/product/830
