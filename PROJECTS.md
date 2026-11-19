# Project ideas: brainstorm

Seeds for Friday's conversation and the week. A good week-long project has a small core
that works by the middle of the week, a demo anyone can try at the end, and stretch goals
for the time in between. Most of these work with only what is on the CLUE; the rest need
one part from the list in [`FRIDAY.md`](FRIDAY.md). Some build on a tab of day 1's sketch
that already does the hard part, named where they do.

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
| Tilt game controller | Tilt the CLUE to steer a ball through a maze on a web page | Day 1's orientation, a browser game | Two players |
| Reaction game | Buttons and screen: who presses first | Buttons, screen, speaker | Leaderboard on the page |
| Step counter | The motion chip counts steps itself; write your own detector and compare | Accelerometer | Steps per minute, distance |
| Tap commands | Tap, double tap and drop do different things | The motion chip's tap and fall detectors | A secret knock |
| Guitar tuner | Names the string you pluck and shows sharp or flat | Microphone: the raw samples from `Sound.ino`, not its spectrum, whose 62.5 Hz steps are far too coarse. Find how long the wave takes to repeat | Any note, sung or whistled; whistle high or low to steer a game on the page |
| Clap-along coach | Shows the tempo of music or of your clapping, and whether you are rushing | Microphone, the beat finder in `Beat.ino` | A metronome you set by tapping the board |
| Air drums | Swing the board like a stick and the laptop plays the drum | Gyro, a web page that makes the sound | Different drums for different directions |
| Posture coach | Clipped to a shirt, it beeps after ten seconds of slouching | Day 1's orientation, speaker | A day's score on the page |
| Elevator profiler | Rides an elevator and plots its speed and height | Pressure sensor, accelerometer | Compare the two sensors' answers; find the fastest elevator |
| Door counter | Counts people passing the board | Proximity sensor | Tell in from out with the gesture sensor |
| Desk guard | Sounds off and tells your phone when someone moves it or opens the drawer it is in | Accelerometer, light sensor, Bluetooth | Arm and disarm with a secret knock |
| Black box | Records a trip in a backpack, a mailed box or a paper rocket to flash; replay it on the page | Accelerometer, pressure sensor, `Storage.ino` | Find the roughest moment |
| Egg and spoon race | Walk a course holding the board level; tilts and jolts cost points | Day 1's orientation, the step counter | Live leaderboard for the whole class |

## With one part from the order

| Project | What it does | Part | Stretch goal |
| --- | --- | --- | --- |
| Theremin | Hand distance sets the pitch | VL53L1X | Second hand controls volume through tilt |
| Parking sensor | Beeps faster as something gets closer | VL53L1X | Distance on the screen |
| Fruit piano | Twelve keys made of bananas, foil or pencil drawings | MPR121 | Record and play back |
| Buzzing compass | Buzzes when facing north; wear it | DRV2605L and motor | Buzz toward a saved spot |
| Silent pager | A phone writes a characteristic and the board buzzes | DRV2605L and motor | Patterns that mean different things |
| Air monitor | CO2 level as green, yellow or red: "open a window" | SCD-43 | A week's log of the classroom |
| Walk tracker | Logs a walk to flash and draws it on a map | PA1010D | Speed and distance |
| Sunburn meter | UV index and how long until a burn | LTR390 | Outdoor log |
| Thermometer gun | Point and read a surface temperature | MLX90632 | Find the coldest window |
| Menu and drawing toy | A dial-driven menu, or an etch-a-sketch on the screen | ANO encoder | A small game |
| Compass ring | The LED pointing north stays lit as the board turns | NeoPixel ring, day 1's orientation | Point to a saved spot |
| Level ring | The ring lights downhill, brighter the steeper the tilt | NeoPixel ring | A two-axis level on the 12 and 16 rings |
| Desk lamp | True white you dim with the touch pads | NeoPixel stick (RGBW) | Warm to cool by mixing in color |
| Light strip | A meter of NeoPixels that reacts to sound or motion | NeoDriver, a strip, the 5 V supply | Music visualizer from the microphone |
| Scrolling sign | Text or a live sensor graph on a 13×9 color matrix | IS31FL3741 | Snake, steered with the ANO dial |
| Spectrum display | The page's dot grid on real LEDs: the board already makes 13 bands and the matrix is 13 wide | IS31FL3741 | Flash the whole matrix on the beat |
| Beat you can feel | The motor taps out the beat of music playing in the room | DRV2605L and motor, the beat finder | A wristband on a battery |
| Catch the light | One LED races round the ring; press the button when it reaches the top | NeoPixel ring | Gets faster each round; two players |
| Timer ring | The ring empties as time runs down, then flashes | NeoPixel ring | Set the time with a tilt or the touch pads |
| Speed trap | Clocks a toy car or a rolling ball from how fast the distance changes | VL53L1X | Chart the run and find the acceleration |
| Water level | Foil strips on the outside of a cup sense the water through it | MPR121 | Fill-me reminder for a plant |
| Plant in a jar | Watch a plant take in CO2 in the light and give it back in the dark | SCD-43 | Compare two plants; the same with a person breathing |
| Sunscreen tester | How much UV gets through sunglasses, a window, or sunscreen on cling film | LTR390 | Rank brands; SPF from the numbers |
| Cooling curve | Logs a hot drink cooling without touching it and predicts when it is drinkable | MLX90632 | Does a lid, a spoon or blowing help most? |
| Satellite clock | The time, to the second, from GPS satellites | PA1010D | Speed and heading for a bike |
| Treasure hunt | An arrow points toward a hidden spot outdoors and says how far | PA1010D, NeoPixel ring or the screen | Hide one for another team |

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
| Room mapper | Turn the board on the spot and the page draws the walls around it | VL53L1X, day 1's orientation | Lining up each distance with the heading it was taken at. Under ordinary room lights the sensor reaches about 3 m off a light wall and under 2 m off a dark one, less near a sunny window, and its beam is 27° wide: good for a small room or a hallway, blurry and short for a classroom. A narrow-beam, longer-range sensor would do a big room |
| Vowel spotter | Tells "ee" from "oo" from "ah" by the shape of the spectrum | Microphone, the spectrum | Voices differ; it needs training on each speaker |
| A better beat finder | Make `Beat.ino` follow rock, jazz or a song with no drums | Microphone, the beat finder | Soft onsets are hard to spot, and real players speed up and slow down |

## Questions to sharpen an idea

- What is the one thing it must do on demo day?
- What can you show working by Wednesday?
- Who is it for, and what would they do with it?
- What would you add with a spare day?
