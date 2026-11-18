// Bluetooth.ino: how the readings get to the page.
//
// What it adds: setupBle() starts the radio and advertises the service. publish() marks a
// reading changed and flushPublished() sends changed readings to whoever subscribed, as
// fast as the radio takes them. noteLinkSettings() reports the radio settings the laptop
// agreed to.
//
// Why: the list of characteristics in day1.ino is our BLE "database": each value lives in
// its characteristic, where anyone can read it, and a page that subscribes is sent each
// change. The library's own notify() waits up to 100 ms when the radio's queue is full,
// which would stall the sensor loop, so this hands values over without waiting. When values
// change faster than the radio can carry them, subscribers skip ahead to the latest and
// stats counts what they missed.

// What the board asks the laptop for once connected. The interval is how often the two
// radios meet, in units of 1.25 ms: each meeting carries only a handful of packets and the
// board has about 130 a second to send. Left alone, Windows settles on 60 ms, about 17
// meetings a second, and readings get skipped; 15 ms is four times that. The timeout is
// how long each side waits in silence before it gives the link up for dead, in units of
// 10 ms. Windows picks 9.6 s, so when a laptop walks off or a page goes away without
// hanging up, the board sat that long before advertising again; 2 s is plenty at 15 ms.
// Each request is made up to LINK_REQUESTS times, one per slow tick, before taking no for
// an answer.
const int LINK_INTERVAL = 12;
const int LINK_TIMEOUT = 200;
const int LINK_REQUESTS = 6;

void setupBle() {
  // Max MTU 247 so the 60-byte vectors packet fits, and room to queue 8 notifications.
  Bluefruit.configPrphConn(247, BLE_GAP_EVENT_LENGTH_DEFAULT, 8, 1);
  Bluefruit.begin();

  sensorService.begin();  // characteristics join the most recently begun service
  for (Published &p : published) {
    beginNotifyChar(p.chr, p.len);
  }

  pixelChar.setProperties(CHR_PROPS_READ | CHR_PROPS_WRITE | CHR_PROPS_WRITE_WO_RESP);
  pixelChar.setFixedLen(3);
  pixelChar.setWriteCallback(onPixelWrite);
  pixelChar.begin();

  beginCalibrationChar();
  beginNameChar();

  // Give each CLUE its own name so a room full of them is easy to tell apart.
  applyName();
  showBleStatus();

  // Our 128-bit UUID nearly fills the advertising packet, so the name goes in the scan
  // response, which scanners ask for right after; applyName() put it there.
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addService(sensorService);
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 32);  // every 20 ms, always, so reconnecting is quick
  Bluefruit.Advertising.start(0);             // 0 = keep advertising until connected
}

// Asks for the 2 Mbit radio mode and a short interval until the laptop or phone agrees,
// then reports the radio settings it agreed to, whenever they change: the MTU
// decides whether the bigger packets arrive whole, the interval is the ceiling on how many
// packets a second get through, the timeout is how long the other side waits in silence
// before it declares the link dead, and the mode is how fast each packet goes over the air.
void noteLinkSettings() {
  static uint16_t lastMtu = 0, lastInterval = 0, lastTimeout = 0;
  static uint8_t lastPhy = 0;
  static int phyRequests = 0, intervalRequests = 0;
  BLEConnection *conn = Bluefruit.Connection(Bluefruit.connHandle());
  if (!Bluefruit.connected() || conn == NULL) {
    lastMtu = 0;
    phyRequests = intervalRequests = 0;
    return;
  }
  // 2 Mbit sends each packet in half the air time. The laptop has the final say. A request
  // is refused while the radio is busy, and fails if it crosses a change the laptop starts
  // at the same moment (Windows changes the interval a second after connecting), so the
  // board keeps asking for a few seconds.
  if (conn->getPHY() != BLE_GAP_PHY_2MBPS && phyRequests < LINK_REQUESTS && conn->requestPHY(BLE_GAP_PHY_2MBPS)) {
    phyRequests++;
  }
  // The same goes for the interval and the timeout, which are asked for together; zero is
  // no skipped meetings. The laptop takes both or neither, so if it has refused half the
  // tries, the rest ask for the interval with the timeout it already has.
  bool tooSlow = conn->getConnectionInterval() > LINK_INTERVAL || conn->getSupervisionTimeout() > LINK_TIMEOUT;
  uint16_t timeout = intervalRequests < LINK_REQUESTS / 2 ? LINK_TIMEOUT : conn->getSupervisionTimeout();
  if (tooSlow && intervalRequests < LINK_REQUESTS && conn->requestConnectionParameter(LINK_INTERVAL, 0, timeout)) {
    intervalRequests++;
  }
  if (conn->getMtu() != lastMtu || conn->getConnectionInterval() != lastInterval ||
      conn->getSupervisionTimeout() != lastTimeout || conn->getPHY() != lastPhy) {
    lastMtu = conn->getMtu();
    lastInterval = conn->getConnectionInterval();
    lastTimeout = conn->getSupervisionTimeout();
    lastPhy = conn->getPHY();
    char message[64];
    snprintf(message, sizeof(message), "Link: MTU %u, interval %.2f ms, timeout %u ms, %u Mbit PHY", lastMtu,
             lastInterval * 1.25, lastTimeout * 10, lastPhy == BLE_GAP_PHY_2MBPS ? 2 : 1);
    note(message);
  }
}

// A characteristic anyone can read or subscribe to, holding a fixed number of bytes.
void beginNotifyChar(BLECharacteristic &chr, int bytes) {
  chr.setProperties(CHR_PROPS_READ | CHR_PROPS_NOTIFY);
  chr.setFixedLen(bytes);
  chr.begin();
}

Published &find(BLECharacteristic &chr) {
  for (Published &p : published) {
    if (&p.chr == &chr) {
      return p;
    }
  }
  halt("publish() of a characteristic that is not in the published table");
  return published[0];
}

// Stores a characteristic's current value and marks it changed. If the previous change
// never reached subscribers, they will skip straight to this one: count that.
void publish(BLECharacteristic &chr) {
  Published &p = find(chr);
  if (p.changed) {
    readings.stats[2]++;
  }
  p.chr.write(p.value, p.len);
  p.changed = true;
}

// Sends changed values to subscribers, stopping when the radio queue is full and picking
// up where it left off on the next loop pass.
void flushPublished() {
  for (Published &p : published) {
    if (!p.changed) {
      continue;
    }
    if (!p.chr.notifyEnabled()) {
      p.changed = false;  // nobody is listening; the value is still there to be read
      continue;
    }
    if (notify(p) == NRF_ERROR_RESOURCES) {
      return;
    }
    p.changed = false;
  }
}

// Hands one value to the radio. Bluefruit's own notify() waits up to 100 ms when the
// queue is full, which would stall the loop; this returns at once either way.
uint32_t notify(Published &p) {
  ble_gatts_hvx_params_t hvx = {};
  uint16_t len = p.len;
  hvx.handle = p.chr.handles().value_handle;
  hvx.type = BLE_GATT_HVX_NOTIFICATION;
  hvx.p_len = &len;
  hvx.p_data = (uint8_t *)p.value;
  return sd_ble_gatts_hvx(Bluefruit.connHandle(), &hvx);
}
