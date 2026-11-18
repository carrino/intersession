// Plugins.ino: notices STEMMA QT boards being plugged in and pulled out.
//
// What it adds: checkPlugins() calls each known I2C address twice a second, logs what
// appeared or went away, and keeps readings.plugins as a bit per board. showPlugins() is
// its line on the screen.
//
// Why: every plug-in board in the class order has its own I2C address, so the address alone
// says what was plugged in. Reading the board is the next step and takes its own library;
// this only tells you the wiring works.

// Each is a bit of readings.plugins, in this order, and orientation.html has the same list.
// The MPR121 and the DRV2605 both answer at 0x5A, so the address cannot tell those apart.
Sensor plugins[] = {
  {"VL53L1X", 0x29, false},         // distance
  {"NeoDriver", 0x60, false},       // NeoPixel strip driver
  {"MPR121/DRV2605", 0x5A, false},  // touch pads / haptic motor
  {"ANO encoder", 0x49, false},     // dial with buttons
  {"PA1010D", 0x10, false},         // GPS
  {"SCD-43", 0x62, false},          // CO2
  {"LTR390", 0x53, false},          // UV
  {"MLX90632", 0x3A, false},        // non-contact temperature
  {"IS31FL3741", 0x30, false},      // 13x9 RGB LED matrix
};

void checkPlugins() {
  char message[64];
  int plugged = 0;
  for (int i = 0; i < (int)(sizeof(plugins) / sizeof(plugins[0])); i++) {
    bool answering = answers(plugins[i].address);
    if (answering != plugins[i].answering) {
      plugins[i].answering = answering;
      snprintf(message, sizeof(message), "%s %s", plugins[i].name, answering ? "plugged in" : "unplugged");
      note(message);
    }
    plugged |= answering << i;
  }
  if (plugged != readings.plugins) {
    readings.plugins = plugged;
    publish(pluginsChar);
  }
}

// The plug-in sensors answering right now, by name.
void showPlugins() {
  bool any = false;
  lineCanvas.print("Plug-in:");
  for (Sensor &plugin : plugins) {
    if (plugin.answering) {
      lineCanvas.print(" ");
      lineCanvas.print(plugin.name);
      any = true;
    }
  }
  if (!any) {
    lineCanvas.print(" none");
  }
}
