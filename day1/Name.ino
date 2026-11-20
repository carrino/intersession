// Name.ino: a name for each board, so a room full of them can be told apart.
//
// What it adds: the name typed on the page is kept in a flash file and the board
// advertises as CLUE-<name>. Until it has one it is CLUE- and the last four digits of its
// Bluetooth address.
//
// Why: eight identical boards all show up in the browser's Bluetooth list at once, and
// everyone needs to find their own.

// Twelve characters is what fits the screen's top line after "CLUE-".
const int MAX_NAME_LEN = 12;
char boardName[MAX_NAME_LEN + 1];  // empty until named
const char *NAME_FILE = "/name.txt";

void loadName() {
  loadFile(NAME_FILE, boardName, MAX_NAME_LEN);
}

void beginNameChar() {
  nameChar.setProperties(CHR_PROPS_READ | CHR_PROPS_WRITE);
  nameChar.setMaxLen(MAX_NAME_LEN);
  nameChar.setWriteCallback(onNameWrite);
  nameChar.begin();
}

// Runs when the page writes a name. An empty one goes back to the default.
void onNameWrite(uint16_t connHandle, BLECharacteristic *chr, uint8_t *data, uint16_t len) {
  len = min(len, (uint16_t)MAX_NAME_LEN);  // setMaxLen() already refuses longer, but be sure
  for (int i = 0; i < len; i++) {
    boardName[i] = isprint(data[i]) ? data[i] : '_';  // nothing that would garble the screen
  }
  boardName[len] = '\0';
  saveFile(NAME_FILE, boardName, len);
  applyName();
  char message[64];
  snprintf(message, sizeof(message), "Renamed %s: scanners see it after this disconnects", bleName);
  note(message);
}

// Advertises as CLUE-<name>, or CLUE- and the end of the Bluetooth address until the
// board has a name. The scan response carries the name, so it is rebuilt too; the board
// sends it the next time it advertises.
void applyName() {
  if (boardName[0] != '\0') {
    snprintf(bleName, sizeof(bleName), "CLUE-%s", boardName);
  } else {
    ble_gap_addr_t addr = Bluefruit.getAddr();
    snprintf(bleName, sizeof(bleName), "CLUE-%02X%02X", addr.addr[1], addr.addr[0]);
  }
  Bluefruit.setName(bleName);
  Bluefruit.ScanResponse.clearData();
  Bluefruit.ScanResponse.addName();
  nameChar.write(boardName);
}
