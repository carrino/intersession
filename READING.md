# Before January: reading and setup

In January we spend a week on the Adafruit CLUE: a small board with a screen, Bluetooth,
and a dozen sensors. You will learn how the electronics work and build a project of your
own. This list gets you ready. Everything on it is free except the books, and the books
are optional.

An hour or two a week between now and January is plenty. Do the setup first.

## 1. Set up your laptop (before winter break)

You need a Windows or Mac laptop you are allowed to install software on. A Chromebook or
an iPad will not work.

1. Follow the setup in the [README](README.md): install the Arduino IDE, the Adafruit
   nRF52 boards, and the Adafruit Arcada library with "Install all". It is a big download,
   so do it at home rather than on school wifi.
2. Download this repository: on its GitHub page, the green **Code** button, then
   **Download ZIP**, and unzip it.
3. Open `day1/day1.ino` from it and press **Verify** (the check mark). If it compiles with
   no errors, your laptop is ready. You do not need a board for this.
4. Make sure you have Chrome or Edge: the class's web page needs one of them.

If anything goes wrong, send the error message; it is much easier to fix now than in
class.

## 2. Programming

The CLUE runs C++, but only a small, plain part of it: variables, `if` and loops,
functions, arrays and structs. You do not need classes, templates, exceptions or the
standard library containers, which take up most of any C++ book.

- **New to programming:** work through [learncpp.com](https://www.learncpp.com/) up to and
  including functions, `if`, loops, arrays and structs. Or, if you like paper, *Programming
  Arduino: Getting Started with Sketches* by Simon Monk, which teaches exactly the part of
  C++ the board uses.
- **Already program in another language:** skim learncpp.com for what is different in
  C++: types and their sizes (`int`, `float`, `uint8_t`), integer division, the bitwise
  operators (`&`, `|`, `<<`, `>>`), and structs.
- Keep the [Arduino language reference](https://docs.arduino.cc/language-reference/)
  handy for board functions like `digitalWrite()` and `millis()`.

## 3. Electronics

Short SparkFun tutorials, grouped by the lecture they set up. Ten to twenty minutes each.

**Lecture 1, electricity:**
[What is a Circuit?](https://learn.sparkfun.com/tutorials/what-is-a-circuit),
[Voltage, Current, Resistance, and Ohm's Law](https://learn.sparkfun.com/tutorials/voltage-current-resistance-and-ohms-law),
[Light-Emitting Diodes](https://learn.sparkfun.com/tutorials/light-emitting-diodes-leds),
[Transistors](https://learn.sparkfun.com/tutorials/transistors) (the part about using one
as a switch), and
[How to Use a Multimeter](https://learn.sparkfun.com/tutorials/how-to-use-a-multimeter).

**Lecture 2, inputs and sensors:**
[Pull-up Resistors](https://learn.sparkfun.com/tutorials/pull-up-resistors),
[Voltage Dividers](https://learn.sparkfun.com/tutorials/voltage-dividers), and
[Analog to Digital Conversion](https://learn.sparkfun.com/tutorials/analog-to-digital-conversion).

**Lecture 3, chips talking to chips:**
[Binary](https://learn.sparkfun.com/tutorials/binary),
[Hexadecimal](https://learn.sparkfun.com/tutorials/hexadecimal),
[Logic Levels](https://learn.sparkfun.com/tutorials/logic-levels),
[I2C](https://learn.sparkfun.com/tutorials/i2c),
[Serial Peripheral Interface (SPI)](https://learn.sparkfun.com/tutorials/serial-peripheral-interface-spi), and
[How to Read a Schematic](https://learn.sparkfun.com/tutorials/how-to-read-a-schematic).

**Lecture 4, time and radio:**
[Pulse Width Modulation](https://learn.sparkfun.com/tutorials/pulse-width-modulation) and
Adafruit's [Introduction to Bluetooth Low Energy](https://learn.adafruit.com/introduction-to-bluetooth-low-energy).

## 4. One book for the break (optional, pick one)

- *Code: The Hidden Language of Computer Hardware and Software* by Charles Petzold
  (2nd edition). How a computer works, from switches and flashlights up to a processor.
  The best single book for this class.
- *Make: Electronics* by Charles Platt (3rd edition). Learn electronics by building small
  circuits, one experiment at a time.
- *Getting Started with Bluetooth Low Energy* by Kevin Townsend and others. For anyone who
  wants to know how the radio part really works.

If you want a full C++ book anyway: *Programming: Principles and Practice Using C++* by
Bjarne Stroustrup, who invented the language. It is long, and most of it goes beyond what
the board uses.

## 5. Watch

- [Visualizing quaternions](https://eater.net/quaternions), by Ben Eater and 3Blue1Brown:
  the math behind how the board knows which way it is pointing.
- Adafruit's [Guide to Excellent Soldering](https://learn.adafruit.com/adafruit-guide-excellent-soldering),
  before you pick up an iron.
