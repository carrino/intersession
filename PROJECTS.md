# Project ideas: brainstorm

Seeds for Friday's conversation and the week. A good week-long project has a small core
that works by the middle of the week, a demo anyone can try at the end, and stretch goals
for the time in between. Most of these work with only what is on the CLUE; the rest need
one sensor from the menu in [`FRIDAY.md`](FRIDAY.md).

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

## With one sensor from the menu

| Project | What it does | Sensor | Stretch goal |
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

## Questions to sharpen an idea

- What is the one thing it must do on demo day?
- What can you show working by Wednesday?
- Who is it for, and what would they do with it?
- What would you add with a spare day?
