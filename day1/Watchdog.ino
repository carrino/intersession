// Watchdog.ino: gets the board going again by itself when the loop stops.
//
// What it adds: stillRunning() is the loop saying it is alive. If it goes quiet for a
// second, the stall check says what the loop was doing (the stage), hangs up the Bluetooth
// link and resets the chip. A hardware watchdog timer does the same after five seconds if
// even that fails. reportLastReset() says after the restart why it happened, and
// unstickI2cBus() frees a sensor left holding the bus.
//
// Why: all the sensors share one I2C bus, and a transfer that is cut short or garbled (an
// upload at the wrong moment, a connector going in, a finger on the edge pads) can leave a
// sensor holding the data line, so the next read waits forever. Without this the screen
// just freezes; with it the board is back in two seconds and tells you where it was stuck.

volatile uint32_t loopAliveMs = 0;  // when loop() last checked in
int stallReboots = 0;               // since power-on; a reset keeps it in the retention register
SoftwareTimer stallTimer;

// Called at the top of every loop pass.
void stillRunning() {
  NRF_WDT->RR[0] = WDT_RR_RR_Reload;  // tell the watchdog we are still running
  loopAliveMs = millis();            // and the stall check
}

void startStallCheck() {
  stallTimer.begin(500, onStallCheck);
  stallTimer.start();
}

// Runs in its own task twice a second. If loop() has not checked in for a second it is
// stuck, almost certainly in an I2C transfer that will never finish. The radio is still
// running, so over the next three ticks: say what it was doing, on the screen and over
// BLE, and keep that in a register that survives the reset; hang up, so the page hears
// at once instead of waiting out its link timeout; then reset the chip.
void onStallCheck(TimerHandle_t timer) {
  static int ticksStuck = 0;
  if (millis() - loopAliveMs < 1000) {
    if (ticksStuck) {
      note("Loop running again");  // slow, then, not stuck
    }
    ticksStuck = 0;
    return;
  }
  ticksStuck++;
  if (ticksStuck == 1) {
    char message[64];
    snprintf(message, sizeof(message), "Stuck in %s", STAGE_NAMES[stage]);
    warn(message);
    notify(find(logChar));  // the stuck loop will not flush it, so send it from here
    // The retention register survives the reset: the stage in the low bits, the count above.
    sd_power_gpregret_clr(1, 0xFF);
    sd_power_gpregret_set(1, stage | min(stallReboots + 1, 15) << 4);
  } else if (ticksStuck == 2) {
    Bluefruit.disconnect(Bluefruit.connHandle());
  } else {
    NVIC_SystemReset();
  }
}

// After a reset, say why if it was ours. Runs before the radio starts, while the retained
// register can still be read directly; the stall check left the stuck stage in it.
void reportLastReset() {
  uint8_t stuck = NRF_POWER->GPREGRET2;
  NRF_POWER->GPREGRET2 = 0;
  stallReboots = stuck >> 4;
  readings.stats[3] = stallReboots;
  if (stuck != 0) {
    char message[64];
    snprintf(message, sizeof(message), "Stall reboot %d: was stuck in %s", stallReboots, STAGE_NAMES[(stuck & 0xF) % STAGE_COUNT]);
    warn(message);
  } else if (readResetReason() & POWER_RESETREAS_DOG_Msk) {
    warn("Rebooted by the watchdog");
  }
}

// The backstop behind the stall check: a hardware timer that resets the chip if loop()
// stops reporting in for five seconds, even if the stall check itself is wedged. The bus
// is cleared on the way back up. A delay() longer than five seconds trips it.
void startWatchdog() {
  NRF_WDT->CONFIG = WDT_CONFIG_SLEEP_Run << WDT_CONFIG_SLEEP_Pos;  // keeps counting while asleep
  NRF_WDT->CRV = 5 * 32768;                                        // five seconds of its 32768 Hz clock
  NRF_WDT->RREN = WDT_RREN_RR0_Msk;
  NRF_WDT->TASKS_START = 1;
}

// A reset in the middle of an I2C read, which every upload risks, leaves that sensor
// holding the data line low until it is clocked out, and every later I2C transfer would
// then wait forever. Pulse the clock by hand until the data line is released, then send
// a stop. Wire takes the pins over afterwards.
void unstickI2cBus() {
  pinMode(PIN_WIRE_SDA, INPUT_PULLUP);
  pinMode(PIN_WIRE_SCL, OUTPUT_S0D1);  // open drain, like an I2C master
  digitalWrite(PIN_WIRE_SCL, HIGH);
  for (int i = 0; i < 9 && digitalRead(PIN_WIRE_SDA) == LOW; i++) {
    digitalWrite(PIN_WIRE_SCL, LOW);
    delayMicroseconds(5);
    digitalWrite(PIN_WIRE_SCL, HIGH);
    delayMicroseconds(5);
  }
  pinMode(PIN_WIRE_SDA, OUTPUT_S0D1);
  digitalWrite(PIN_WIRE_SDA, LOW);  // stop condition: data rises while the clock is high
  delayMicroseconds(5);
  digitalWrite(PIN_WIRE_SDA, HIGH);
}
