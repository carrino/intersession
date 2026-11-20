# Working in this repo

- Commit and push to the current branch and its pull request without asking. Do not merge.
- This is code for a week-long electronics class on the Adafruit CLUE. Keep it readable,
  minimal and cleanly factored; a student should be able to digest a day's sketch.
- Follow the existing style: braces on every block, small helpers with a one-line comment
  saying why, constants at the top of the sketch.
- A feature that takes more than a library call gets its own tab beside the sketch
  (`day1/Touch.ino`), starting with a header that says what it adds and why. A tab keeps its
  own constants and variables at its top, and the rest of the program uses it only through
  its functions; anything shared lives in the main sketch. The main sketch and the day's
  own README (`day1/README.md`) each list the tabs.
- Each day's sketch lives in its own folder named after the sketch (`day1/day1.ino`), with
  its companion files beside it. `README.md` has getting started and the problems you are
  likely to hit, `TODO.md` the backlog, `PACKING.md` what to bring to the class,
  `SLIDES.md` the slide topics being brainstormed,
  `FRIDAY.md` the first meeting's talking points, `PROJECTS.md` project ideas,
  `READING.md` the students' reading list and `datasheets/` every part's datasheet plus
  the CLUE's schematic and pinout.
