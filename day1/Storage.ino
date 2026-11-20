// Storage.ino: small files that keep their contents with the power off.
//
// What it adds: loadFile() and saveFile(), which the calibration and the board's name are
// kept with. The files live in a 28 kB corner of the chip's own flash, run as a tiny file
// system (LittleFS) by the board package.
//
// Why saveFile() takes three steps: so a save is atomic. Whenever the power goes, even in
// the middle of a save, the file afterwards holds all of the old contents or all of the
// new, never half of each and never nothing. Writing over the old file cannot promise
// that, and neither can deleting it first. So the new contents go into a second file, and
// only once that is completely written is it renamed over the first. The rename is one
// small change to the file system's directory, which LittleFS makes safely: it keeps two
// copies of each directory block with a checksum and writes them in turn, so a change cut
// short leaves the older copy in charge. Databases, phones and laptops save things the
// same way.

const char *SAVING_FILE = "/saving.tmp";  // where new contents go until they are complete

void setupStorage() {
  InternalFS.begin();  // on a board's first run this formats the flash
}

// Reads up to len bytes of a flash file into data, leaving data as it was if there is none.
void loadFile(const char *path, void *data, int len) {
  Adafruit_LittleFS_Namespace::File file(InternalFS);
  if (file.open(path, Adafruit_LittleFS_Namespace::FILE_O_READ)) {
    file.read(data, len);
    file.close();
  }
}

// Replaces a flash file's contents, atomically. Returns whether it was saved.
bool saveFile(const char *path, const void *data, int len) {
  // What a save that was cut short left behind. Opening a file to write adds to its end.
  InternalFS.remove(SAVING_FILE);
  Adafruit_LittleFS_Namespace::File file(InternalFS);
  if (!file.open(SAVING_FILE, Adafruit_LittleFS_Namespace::FILE_O_WRITE)) {
    return false;
  }
  bool complete = (int)file.write((const uint8_t *)data, len) == len;
  file.close();
  return complete && InternalFS.rename(SAVING_FILE, path);  // the moment the new contents take over
}
