// DRUM CLOCK + SOLAR ORRERY - 320x240 ILI9341, ESP32
// Hardware buttons move between pages. Touch is reserved for interactive pages.
//main.ino
#include "AppConfig.h"

// Credentials
const char* ssid          = "YOUR_WIFI_SSID";
const char* password      = "YOUR_WIFI_PASSWORD";
String      apiKey        = "YOUR_OPENWEATHERMAP_API_KEY";
String      city          = "YOUR_CITY";
const char* ntpServer     = "pool.ntp.org";
const long  gmtOffset_sec = 19800;

DisplayCompat tft;
XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ);

ScreenMode currentScreen = SCREEN_CLOCK;
unsigned long lastTouchMs = 0;

uint16_t C_BG, C_PUR, C_PUR2, C_PUR_DIM, C_WHITE, C_YELLOW, C_GREEN;

static bool touchWasDown = false;
static int touchStartX = 0;
static int touchStartY = 0;
static int touchLastX = 0;
static int touchLastY = 0;
static unsigned long touchDownSince = 0;
static int pendingTouchAction = TOUCH_NONE;
static unsigned long lastButtonMs = 0;

void initColours() {
  C_BG      = tft.color565(0,   0,   0);
  C_PUR     = tft.color565(140, 0, 210);
  C_PUR2    = tft.color565(100, 0, 160);
  C_PUR_DIM = tft.color565(55,  0,  88);
  C_WHITE   = TFT_WHITE;
  C_YELLOW  = TFT_YELLOW;
  C_GREEN   = TFT_GREEN;
}

bool readTouchPoint(int& x, int& y) {
  if (!ts.touched()) return false;

  TS_Point p = ts.getPoint();
  x = map(p.x, TOUCH_X_MIN, TOUCH_X_MAX, 0, SW);
  y = map(p.y, TOUCH_Y_MIN, TOUCH_Y_MAX, 0, SH);
  x = constrain(x, 0, SW - 1);
  y = constrain(y, 0, SH - 1);
  return true;
}

int classifyTouch(int startX, int startY, int endX, int endY) {
  int dx = endX - startX;
  int dy = endY - startY;

  if (abs(dx) > TOUCH_SWIPE_PIXELS || abs(dy) > TOUCH_SWIPE_PIXELS) {
    if (abs(dx) > abs(dy)) return dx > 0 ? TOUCH_RIGHT : TOUCH_LEFT;
    return dy > 0 ? TOUCH_DOWN : TOUCH_UP;
  }

  if (endY < 42) {
    return TOUCH_UP;
  }
  if (endY > SH - 46) return TOUCH_DOWN;
  if (endX < 70) return TOUCH_LEFT;
  if (endX > SW - 70) return TOUCH_RIGHT;
  return TOUCH_SELECT;
}

int readTouchAction() {
  if (pendingTouchAction != TOUCH_NONE) return pendingTouchAction;

  int x, y;
  bool down = readTouchPoint(x, y);

  if (down && !touchWasDown) {
    touchStartX = touchLastX = x;
    touchStartY = touchLastY = y;
    touchDownSince = millis();
    touchWasDown = true;
    return TOUCH_NONE;
  }

  if (down) {
    if (millis() - touchDownSince > TOUCH_HOLD_TIMEOUT_MS) {
      resetTouchState();
      lastTouchMs = millis();
      return TOUCH_NONE;
    }
    touchLastX = x;
    touchLastY = y;
    return TOUCH_NONE;
  }

  if (touchWasDown) {
    touchWasDown = false;
    unsigned long now = millis();
    if (now - lastTouchMs < DEBOUNCE_MS) return TOUCH_NONE;
    lastTouchMs = now;
    pendingTouchAction = classifyTouch(touchStartX, touchStartY, touchLastX, touchLastY);
  }

  return pendingTouchAction;
}

bool touchPressed(int action) {
  int got = readTouchAction();
  if (got != action) return false;
  pendingTouchAction = TOUCH_NONE;
  return true;
}

void resetTouchState() {
  touchWasDown = false;
  touchDownSince = 0;
  pendingTouchAction = TOUCH_NONE;
}

bool buttonPressed(int action) {
  int pin = -1;
  if (action == TOUCH_UP) pin = BTN_UP;
  else if (action == TOUCH_SELECT) pin = BTN_SELECT;
  else if (action == TOUCH_DOWN) pin = BTN_DOWN;
  else return false;

  if (digitalRead(pin) != LOW) return false;

  unsigned long now = millis();
  if (now - lastButtonMs < BUTTON_DEBOUNCE_MS) return false;
  lastButtonMs = now;

  while (digitalRead(pin) == LOW) delay(5);
  return true;
}

int nextScreenFrom(int screen, int direction) {
  int idx = screen + direction;
  if (idx < (int)SCREEN_CLOCK) idx = (int)SCREEN_SYSTEM;
  if (idx > (int)SCREEN_SYSTEM) idx = (int)SCREEN_CLOCK;
  return idx;
}

void renderScreenSnapshot(int screen) {
  if (screen == SCREEN_CLOCK) renderClockSnapshot();
  else if (screen == SCREEN_WEATHER) renderWeatherSnapshot();
  else if (screen == SCREEN_SOLAR) renderSolarSnapshot();
  else if (screen == SCREEN_SNAKE) renderSnakeSnapshot();
  else if (screen == SCREEN_DOODLE) renderDoodleSnapshot();
  else renderSystemSnapshot();
}

void enterScreen(int screen) {
  if (screen == SCREEN_CLOCK) showClockScreen();
  else if (screen == SCREEN_WEATHER) showWeatherScreen();
  else if (screen == SCREEN_SOLAR) showSolarScreen();
  else if (screen == SCREEN_SNAKE) showSnakeScreen();
  else if (screen == SCREEN_DOODLE) showDoodleScreen();
  else showSystemScreen();
}

void navigateScreens(int direction) {
  int from = currentScreen;
  int to = nextScreenFrom(from, direction);

  // The canvas needs 320*240*2 = 153,600 bytes. When WiFi is active the
  // ESP32-C3 heap is tight (~160-200 KB free). Guard against OOM:
  // if there is not enough free contiguous heap, skip the animation.
  const size_t CANVAS_BYTES = (size_t)SW * SH * 2;
  if (ESP.getMaxAllocHeap() < CANVAS_BYTES + 16384) {
    // Not enough room — enter directly without the scroll animation
    enterScreen(to);
    resetTouchState();
    return;
  }

  GFXcanvas16 oldPage(SW, SH);

  if (!oldPage.getBuffer()) {
    enterScreen(to);
    resetTouchState();
    return;
  }

  tft.setRenderTarget(&oldPage);
  renderScreenSnapshot(from);
  tft.setRenderTarget(nullptr);

  enterScreen(to);

  for (int i = 0; i <= PAGE_SCROLL_STEPS; i++) {
    int eased = (i * i * (3 * PAGE_SCROLL_STEPS - 2 * i)) /
                (PAGE_SCROLL_STEPS * PAGE_SCROLL_STEPS);
    int offset = direction > 0 ? -eased * SH / PAGE_SCROLL_STEPS
                               :  eased * SH / PAGE_SCROLL_STEPS;

    renderScreenSnapshot(to);
    tft.drawRGBBitmap(0, offset, oldPage.getBuffer(), SW, SH);
    delay(PAGE_SCROLL_DELAY_MS);
  }

  renderScreenSnapshot(to);
  resetTouchState();
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println();
  Serial.println("Booting solar_orrery ESP32-C3");

  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
  pinMode(TFT_CS, OUTPUT);
  pinMode(TOUCH_CS, OUTPUT);
  digitalWrite(TFT_CS, HIGH);
  digitalWrite(TOUCH_CS, HIGH);
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);

  tft.init();
  tft.setRotation(3);
  ts.begin();
  ts.setRotation(3);

  initColours();
  initSolarColours();
  initOrbits();
  bootSequence();
  initSprites();
  showClockScreen();
}

void loop() {
  if (buttonPressed(TOUCH_DOWN)) {
    navigateScreens(1);
    return;
  }

  if (buttonPressed(TOUCH_UP)) {
    navigateScreens(-1);
    return;
  }

  if (currentScreen != SCREEN_SNAKE && currentScreen != SCREEN_DOODLE &&
      currentScreen != SCREEN_SOLAR) {
    resetTouchState();
  }

  if (currentScreen == SCREEN_CLOCK) updateClockScreen();
  else if (currentScreen == SCREEN_WEATHER) updateWeatherScreen();
  else if (currentScreen == SCREEN_SOLAR) updateSolarScreen();
  else if (currentScreen == SCREEN_SNAKE) updateSnakeScreen();
  else if (currentScreen == SCREEN_DOODLE) updateDoodleScreen();
  else updateSystemScreen();

  delay(50);
}
