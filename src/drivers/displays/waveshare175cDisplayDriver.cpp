#include "displayDriver.h"
#ifdef WAVESHARE_175C_DISPLAY
#include <Arduino_GFX_Library.h>
#include <TouchDrvCSTXXX.hpp>
#include <Wire.h>
#include <WiFi.h>
#include "display.h"
#include "monitor.h"

namespace {
Arduino_ESP32QSPI bus(12, 38, 4, 5, 6, 7);
Arduino_CO5300 lcd(&bus, 1, 0, false, 466, 466, 6, 0, 0, 0);
TouchDrvCST92xx touch;
bool ready = false, touchReady = false, enabled = true, held = false;
uint8_t rotation = 0;
uint32_t touchStarted = 0;
bool touchHandled = false;
// Compose each changed row in RAM before sending it to the panel.
Arduino_Canvas rowCanvas(466, 32, nullptr);
String currentTitle;
struct TextRow {
  int y = -1;
  String value;
  uint16_t width = 0;
  uint8_t size = 0;
  uint16_t color = 0;
};
TextRow rows[12];
void text(const String &value, int y, uint8_t size = 2, uint16_t color = 0xffff) {
  TextRow *row = nullptr;
  for (auto &entry : rows) {
    if (entry.y == y) { row = &entry; break; }
    if (!row && entry.y == -1) row = &entry;
  }
  if (!row) return;
  if (row->y == y && row->value == value && row->size == size && row->color == color) return;
  rowCanvas.setTextSize(size);
  rowCanvas.setTextColor(color, 0);
  rowCanvas.setTextWrap(false);
  int16_t x, yy; uint16_t w, h;
  rowCanvas.getTextBounds(value, 0, 0, &x, &yy, &w, &h);
  uint16_t width = max(w, row->width);
  if (width > 466) width = 466;
  const uint16_t height = 8 * max(size, row->size);
  rowCanvas.fillScreen(0);
  rowCanvas.setCursor((width - w) / 2, 0);
  rowCanvas.print(value);
  uint16_t left = 0, right = width;
  // With a stable field width, leave labels, units and unchanged digits alone.
  if (row->y == y && row->size == size && row->color == color && row->width == w) {
    unsigned int first = 0, last = value.length();
    while (first < last && value[first] == row->value[first]) ++first;
    while (last > first && value[last - 1] == row->value[last - 1]) --last;
    left = min<uint16_t>(width, first * 6 * size);
    right = min<uint16_t>(width, last * 6 * size);
  }
  const uint16_t dirtyWidth = right - left;
  uint16_t *pixels = rowCanvas.getFramebuffer();
  // Transfer only the changed character cells, with no visible clearing pass.
  for (uint16_t line = 0; line < height; ++line)
    memmove(pixels + line * dirtyWidth, pixels + line * 466 + left, dirtyWidth * sizeof(uint16_t));
  if (dirtyWidth) lcd.draw16bitRGBBitmap((466 - width) / 2 + left, y, pixels, dirtyWidth, height);
  row->y = y;
  row->value = value;
  row->width = w;
  row->size = size;
  row->color = color;
}
void base(const char *title) {
  if (currentTitle == title) return;
  currentTitle = title;
  for (auto &row : rows) row = TextRow{};
  lcd.fillScreen(0);
  lcd.drawCircle(233, 233, 225, 0x03ef);
  text("NERDMINER", 48, 3, 0xfd20);
  text(title, 91);
}
void initPanel() {
  Wire.begin(15, 14);
  Wire.setClock(400000);
  ready = lcd.begin();
  ready = rowCanvas.begin() && ready;
  if (!ready) Serial.println("CO5300 initialization failed");
  else {
    lcd.setBrightness(128);
    Serial.printf("CO5300 ready: %d x %d\n", lcd.width(), lcd.height());
  }
  touch.setPins(2, 11);
  touchReady = touch.begin(Wire, 0x5a, 15, 14);
  if (touchReady) {
    touch.setMaxCoordinates(466, 466);
    touch.setMirrorXY(true, true);
  }
  Serial.printf("CST9217: %s\n", touchReady ? "ready" : "not detected");
}
void state() {
  if (!ready) return;
  enabled = !enabled;
  if (enabled) {
    lcd.displayOn();
    lcd.setBrightness(128);
  } else {
    lcd.setBrightness(0);
    lcd.displayOff();
  }
  Serial.printf("Display: %s\n", enabled ? "on" : "off (mining continues)");
}
void rotate() {
  rotation ^= 2;
  if (ready) lcd.setRotation(rotation);
  currentTitle = "";
}
void loading() { if (ready) { base("ESP32-S3 AMOLED 1.75C"); text("Starting...", 215, 3); } }
void setupScreen() {
  if (!ready) return;
  base("WI-FI SETUP");
  text(WiFi.softAPSSID(), 175);
  text("Open in browser:", 220);
  text(WiFi.softAPIP().toString(), 255, 3, 0x07ff);
}
String field(String value, unsigned int length) {
  while (value.length() < length) value = " " + value;
  return value;
}
void miner(unsigned long elapsed) {
  auto d = getMiningData(elapsed);
  if (!ready || !enabled) return;
  base("MINING");
  text(field(d.currentHashRate, 6) + " KH/s", 148, 4, 0x07ff);
  text("Shares: " + field(d.completedShares, 8), 212);
  text("Best: " + field(d.bestDiff, 10), 247);
  text("Uptime: " + field(d.timeMining, 15), 282);
  text("Temp: " + field(d.temp, 3), 317);
  text("Tap: next screen", 360, 2, 0x7bef);
  text("Hold: screen off", 390, 2, 0x7bef);
}
void clockScreen(unsigned long elapsed) {
  auto d = getClockData(elapsed);
  if (!ready || !enabled) return;
  base("BITCOIN / CLOCK");
  text(d.currentTime, 155, 4, 0x07ff);
  text(d.currentDate, 215);
  text("BTC: " + field(d.btcPrice, 10), 260);
  text("Block: " + field(d.blockHeight, 9), 300);
  text("Tap: next screen", 360, 2, 0x7bef);
  text("Hold: screen off", 390, 2, 0x7bef);
}
// Poll on the monitor task: display state and I2C stay on one task.
void animate(unsigned long) {
  if (!touchReady) return;
  int16_t x[5] = {}, y[5] = {};
  bool down = touch.getPoint(x, y, touch.getSupportTouchPoint()) > 0;
  const uint32_t now = millis();
  if (down && !held) {
    touchStarted = now;
    touchHandled = false;
    if (!enabled) { state(); touchHandled = true; }
  }
  if (down && !touchHandled && now - touchStarted >= 1000) {
    state();
    touchHandled = true;
  }
  if (!down && held && !touchHandled) switchToNextScreen();
  held = down;
}
void led(unsigned long) {}
CyclicScreenFunction screens[] = {miner, clockScreen};
}
DisplayDriver waveshare175cDriver = {
  initPanel, state, rotate, loading, setupScreen, screens, animate, led,
  SCREENS_ARRAY_SIZE(screens), 0, 466, 466
};
#endif

