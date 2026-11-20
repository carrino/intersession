# Before January: reading and setup

In January we spend a week on the Adafruit CLUE: a small board with a screen, Bluetooth,
and a dozen sensors. You will learn how the electronics work and build a project of your
own. This list gets you ready. Everything on it is free except the books, and the books
are optional.

An hour or two a week between now and January is plenty. Do the setup first.

## 1. Set up your laptop (before winter break)

You need a Windows or Mac laptop you are allowed to install software on. A Chromebook or
an iPad will not work.

1. Follow **Set up your laptop** in the [README](README.md). It has you install the
   Arduino IDE and the board's files (a big download, so do it at home rather than on
   school wifi), make your own copy of this repository on GitHub, called a fork, and
   bring that copy down to your laptop, called a clone.
2. Its last step is to open `day1/day1.ino` from your clone and press **Verify** (the check
   mark). If it compiles with no errors, your laptop is ready. You do not need a board for
   this.

If anything goes wrong, send the error message; it is much easier to fix now than in
class.

**Why a fork and not a ZIP?** In January you will be changing this code every day. With a
fork, every change is saved with a note saying what it was and why, you can go back to
any earlier version when something breaks, your work is backed up off your laptop, and you
can take in fixes from the class repository without losing your own changes. Software
everywhere is built this way, and the habits outlast any one project:

- Commit small and often: one change, one commit.
- Say why in the message, not only what.
- Push before you close the laptop.

## 2. C++: a short reading list

The CLUE runs C++, but only a part of it. Templates, exceptions, inheritance and the
standard library containers take up much of any C++ book, and they can all wait. These
lessons from [learncpp.com](https://www.learncpp.com/) cover what the board uses, about
ten minutes each; skip everything else on the site for now. The lesson numbers are the
site's own.

learncpp's examples run on a computer and print with `std::cout`; on the board,
`Serial.print()` and the screen do that job. To try an example, paste it into an online
compiler such as [OnlineGDB](https://www.onlinegdb.com/online_c++_compiler).

**Variables and functions**
- 1.1 [Statements and the structure of a program](https://www.learncpp.com/cpp-tutorial/statements-and-the-structure-of-a-program/)
- 1.3 [Introduction to objects and variables](https://www.learncpp.com/cpp-tutorial/introduction-to-objects-and-variables/)
- 2.1 [Introduction to functions](https://www.learncpp.com/cpp-tutorial/introduction-to-functions/)
- 2.4 [Introduction to function parameters and arguments](https://www.learncpp.com/cpp-tutorial/introduction-to-function-parameters-and-arguments/)

**Numbers** (the board's code is full of `uint8_t`, `uint32_t` and `float`)
- 4.6 [Fixed-width integers and size_t](https://www.learncpp.com/cpp-tutorial/fixed-width-integers-and-size-t/)
- 4.8 [Floating point numbers](https://www.learncpp.com/cpp-tutorial/floating-point-numbers/)
- 6.2 [Arithmetic operators](https://www.learncpp.com/cpp-tutorial/arithmetic-operators/) (watch for integer division)
- 5.1 [Constant variables (named constants)](https://www.learncpp.com/cpp-tutorial/constant-variables-named-constants/)

**Decisions and loops**
- 8.2 [If statements and blocks](https://www.learncpp.com/cpp-tutorial/if-statements-and-blocks/)
- 8.5 [Switch statement basics](https://www.learncpp.com/cpp-tutorial/switch-statement-basics/)
- 8.10 [For statements](https://www.learncpp.com/cpp-tutorial/for-statements/)
- 16.8 [Range-based for loops (for-each)](https://www.learncpp.com/cpp-tutorial/range-based-for-loops-for-each/)

**Where variables live** (`loop()` runs forever, so this matters)
- 7.4 [Introduction to global variables](https://www.learncpp.com/cpp-tutorial/introduction-to-global-variables/)
- 7.11 [Static local variables](https://www.learncpp.com/cpp-tutorial/static-local-variables/)

**Grouping data**
- 17.7 [Introduction to C-style arrays](https://www.learncpp.com/cpp-tutorial/introduction-to-c-style-arrays/)
- 13.7 [Introduction to structs, members, and member selection](https://www.learncpp.com/cpp-tutorial/introduction-to-structs-members-and-member-selection/)
- 13.2 [Unscoped enumerations](https://www.learncpp.com/cpp-tutorial/unscoped-enumerations/)

**Classes** (every sensor library is one, and so are the board's `Quaternion` and `MagCalibration`)
- 14.1 [Introduction to object-oriented programming](https://www.learncpp.com/cpp-tutorial/introduction-to-object-oriented-programming/)
- 14.2 [Introduction to classes](https://www.learncpp.com/cpp-tutorial/introduction-to-classes/)
- 14.3 [Member functions](https://www.learncpp.com/cpp-tutorial/member-functions/)
- 14.4 [Const class objects and const member functions](https://www.learncpp.com/cpp-tutorial/const-class-objects-and-const-member-functions/)
- 14.5 [Public and private members and access specifiers](https://www.learncpp.com/cpp-tutorial/public-and-private-members-and-access-specifiers/)
- 14.9 [Introduction to constructors](https://www.learncpp.com/cpp-tutorial/introduction-to-constructors/)
- 15.2 [Classes and header files](https://www.learncpp.com/cpp-tutorial/classes-and-header-files/)
- 21.1 [Introduction to operator overloading](https://www.learncpp.com/cpp-tutorial/introduction-to-operator-overloading/) (how `orientation * turn` multiplies two quaternions)

**References and pointers** (just the introductions)
- 12.3 [Lvalue references](https://www.learncpp.com/cpp-tutorial/lvalue-references/)
- 12.7 [Introduction to pointers](https://www.learncpp.com/cpp-tutorial/introduction-to-pointers/)
- 17.8 [C-style array decay](https://www.learncpp.com/cpp-tutorial/c-style-array-decay/)

**Bits** (how one byte holds both buttons)
- O.2 [Bitwise operators](https://www.learncpp.com/cpp-tutorial/bitwise-operators/)
- O.3 [Bit manipulation with bitwise operators and bit masks](https://www.learncpp.com/cpp-tutorial/bit-manipulation-with-bitwise-operators-and-bit-masks/)

**Then read real code:** open `day1/day1.ino` and its tabs and find a struct, an enum, a
`switch`, a static local variable, a range-based `for`, and a bit mask. Then open
`day1/Quaternion.h` and `day1/MagCalibration.h` and find a constructor, a const member
function, a private member, and an overloaded operator. Much of the rest is calling the
libraries' objects, like `display.print()`, which is a member function of a class someone
else wrote.

If you already program in another language, skim the list for what is different in C++.
If you like paper, *Programming Arduino: Getting Started with Sketches* by Simon Monk
covers the same ground. Keep the [Arduino language reference](https://docs.arduino.cc/language-reference/)
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

**Lecture 5, the physics inside:**
[Accelerometer Basics](https://learn.sparkfun.com/tutorials/accelerometer-basics),
[Gyroscope](https://learn.sparkfun.com/tutorials/gyroscope), and
[Capacitors](https://learn.sparkfun.com/tutorials/capacitors), since so many sensors turn
out to be one.

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
