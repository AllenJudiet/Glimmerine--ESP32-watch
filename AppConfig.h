#pragma once

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <esp_wifi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Esp.h>
#include "time.h"
#include <math.h>
#include "orbital.h"

// ESP32 + shared SPI bus pin map from the ILI9341/XPT2046 wiring.
#define TFT_CS     5
#define TFT_DC     2
#define TFT_RST    4
#define TOUCH_CS   15
#define TOUCH_IRQ  27
#define SPI_SCK    18
#define SPI_MISO   19
#define SPI_MOSI   23
#define BTN_UP     26
#define BTN_SELECT 25
#define BTN_DOWN   33

// XPT2046 calibration for landscape rotation.
#define TOUCH_X_MIN 3819
#define TOUCH_X_MAX 457
#define TOUCH_Y_MIN 371
#define TOUCH_Y_MAX 3754

// Display
#define SW 320
#define SH 240

#define TFT_WHITE  ILI9341_WHITE
#define TFT_YELLOW ILI9341_YELLOW
#define TFT_GREEN  ILI9341_GREEN

#define TL_DATUM 0
#define ML_DATUM 1
#define MC_DATUM 2
#define MR_DATUM 3

class DisplayCompat : public Adafruit_ILI9341 {
public:
  DisplayCompat() : Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST) {}

  void init() { begin(); }
  void setRenderTarget(GFXcanvas16* target) { renderTarget = target; }

  void fillScreen(uint16_t color) {
    if (renderTarget) renderTarget->fillScreen(color);
    else Adafruit_ILI9341::fillScreen(color);
  }

  void drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color) {
    if (renderTarget) renderTarget->drawRoundRect(x, y, w, h, r, color);
    else Adafruit_ILI9341::drawRoundRect(x, y, w, h, r, color);
  }

  void fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color) {
    if (renderTarget) renderTarget->fillRoundRect(x, y, w, h, r, color);
    else Adafruit_ILI9341::fillRoundRect(x, y, w, h, r, color);
  }

  void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    if (renderTarget) renderTarget->drawRect(x, y, w, h, color);
    else Adafruit_ILI9341::drawRect(x, y, w, h, color);
  }

  void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    if (renderTarget) renderTarget->fillRect(x, y, w, h, color);
    else Adafruit_ILI9341::fillRect(x, y, w, h, color);
  }

  void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {
    if (renderTarget) renderTarget->drawFastHLine(x, y, w, color);
    else Adafruit_ILI9341::drawFastHLine(x, y, w, color);
  }

  void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) {
    if (renderTarget) renderTarget->drawFastVLine(x, y, h, color);
    else Adafruit_ILI9341::drawFastVLine(x, y, h, color);
  }

  void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
    if (renderTarget) renderTarget->drawLine(x0, y0, x1, y1, color);
    else Adafruit_ILI9341::drawLine(x0, y0, x1, y1, color);
  }

  void drawCircle(int16_t x, int16_t y, int16_t r, uint16_t color) {
    if (renderTarget) renderTarget->drawCircle(x, y, r, color);
    else Adafruit_ILI9341::drawCircle(x, y, r, color);
  }

  void fillCircle(int16_t x, int16_t y, int16_t r, uint16_t color) {
    if (renderTarget) renderTarget->fillCircle(x, y, r, color);
    else Adafruit_ILI9341::fillCircle(x, y, r, color);
  }

  void drawRGBBitmap(int16_t x, int16_t y, uint16_t* bitmap, int16_t w, int16_t h) {
    if (renderTarget) renderTarget->drawRGBBitmap(x, y, bitmap, w, h);
    else Adafruit_ILI9341::drawRGBBitmap(x, y, bitmap, w, h);
  }

  void setTextColor(uint16_t color, uint16_t bg) {
    if (renderTarget) renderTarget->setTextColor(color, bg);
    else Adafruit_ILI9341::setTextColor(color, bg);
  }

  void setTextFont(uint8_t font) {
    textSizeCompat = font >= 6 ? 4 : (font >= 4 ? 3 : (font >= 2 ? 2 : 1));
    if (renderTarget) renderTarget->setTextSize(textSizeCompat);
    else Adafruit_ILI9341::setTextSize(textSizeCompat);
  }

  void setTextDatum(uint8_t datum) { datumCompat = datum; }

  int textWidth(const String& s) {
    int16_t x1, y1;
    uint16_t w, h;
    if (renderTarget) renderTarget->getTextBounds(s, 0, 0, &x1, &y1, &w, &h);
    else Adafruit_ILI9341::getTextBounds(s, 0, 0, &x1, &y1, &w, &h);
    return w;
  }

  int drawString(const String& s, int x, int y) {
    int16_t tx = x;
    int16_t ty = y;
    uint16_t w = textWidth(s);
    uint16_t h = 8 * textSizeCompat;
    if (datumCompat == MC_DATUM) {
      tx -= w / 2;
      ty -= h / 2;
    } else if (datumCompat == ML_DATUM) {
      ty -= h / 2;
    } else if (datumCompat == MR_DATUM) {
      tx -= w;
      ty -= h / 2;
    }
    if (renderTarget) {
      renderTarget->setCursor(tx, ty);
      renderTarget->print(s);
    } else {
      Adafruit_ILI9341::setCursor(tx, ty);
      Adafruit_ILI9341::print(s);
    }
    return w;
  }

  int drawString(const char* s, int x, int y) { return drawString(String(s), x, y); }

private:
  GFXcanvas16* renderTarget = nullptr;
  uint8_t datumCompat = TL_DATUM;
  uint8_t textSizeCompat = 1;
};

class TFT_eSprite {
public:
  explicit TFT_eSprite(DisplayCompat* parent) : tft(parent) {}

  void createSprite(int16_t w, int16_t h) {
    delete canvas;
    width = w;
    height = h;
    canvas = new GFXcanvas16(w, h);
  }

  void setColorDepth(uint8_t depth) {}
  void fillSprite(uint16_t color) { if (canvas) canvas->fillScreen(color); }
  void fillScreen(uint16_t color) { fillSprite(color); }
  void fillCircle(int16_t x, int16_t y, int16_t r, uint16_t color) { canvas->fillCircle(x, y, r, color); }
  void drawCircle(int16_t x, int16_t y, int16_t r, uint16_t color) { canvas->drawCircle(x, y, r, color); }
  void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) { canvas->fillRect(x, y, w, h, color); }
  void fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color) { canvas->fillTriangle(x0, y0, x1, y1, x2, y2, color); }
  void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) { canvas->drawFastHLine(x, y, w, color); }
  void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) { canvas->drawFastVLine(x, y, h, color); }
  void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) { canvas->drawLine(x0, y0, x1, y1, color); }
  void setTextColor(uint16_t color, uint16_t bg) { canvas->setTextColor(color, bg); }
  void setTextFont(uint8_t font) {
    textSizeCompat = font >= 6 ? 4 : (font >= 4 ? 3 : (font >= 2 ? 2 : 1));
    canvas->setTextSize(textSizeCompat);
  }
  void setTextDatum(uint8_t datum) { datumCompat = datum; }

  int textWidth(const String& s) {
    int16_t x1, y1;
    uint16_t w, h;
    canvas->getTextBounds(s, 0, 0, &x1, &y1, &w, &h);
    return w;
  }

  int drawString(const String& s, int x, int y) {
    int16_t tx = x;
    int16_t ty = y;
    uint16_t w = textWidth(s);
    uint16_t h = 8 * textSizeCompat;
    if (datumCompat == MC_DATUM) {
      tx -= w / 2;
      ty -= h / 2;
    } else if (datumCompat == ML_DATUM) {
      ty -= h / 2;
    } else if (datumCompat == MR_DATUM) {
      tx -= w;
      ty -= h / 2;
    }
    canvas->setCursor(tx, ty);
    canvas->print(s);
    return w;
  }

  int drawString(const char* s, int x, int y) { return drawString(String(s), x, y); }

  void pushSprite(int16_t x, int16_t y) {
    if (canvas) tft->drawRGBBitmap(x, y, canvas->getBuffer(), width, height);
  }

private:
  DisplayCompat* tft;
  GFXcanvas16* canvas = nullptr;
  int16_t width = 0;
  int16_t height = 0;
  uint8_t datumCompat = TL_DATUM;
  uint8_t textSizeCompat = 1;
};

// Touch/button control actions. Touch is kept for in-screen controls.
enum TouchAction {
  TOUCH_NONE,
  TOUCH_LEFT,
  TOUCH_RIGHT,
  TOUCH_UP,
  TOUCH_DOWN,
  TOUCH_SELECT
};

#define DEBOUNCE_MS          180
#define BUTTON_DEBOUNCE_MS   140
#define TOUCH_SWIPE_PIXELS    35
#define TOUCH_HOLD_TIMEOUT_MS 1500
#define PAGE_SCROLL_STEPS      18
#define PAGE_SCROLL_DELAY_MS    8
#define NUM_DEAUTH_PACKETS   128
#define SPACE_WEATHER_INTERVAL_SEC 300

// Weather/time
#define WEATHER_INTERVAL_SEC 300

// Clock layout
#define OB_M  5
#define OB_R  8

#define TB_X  (OB_M + 1)
#define TB_Y  (OB_M + 1)
#define TB_W  (SW - 2*(OB_M+1))
#define TB_H  22

#define BB_H  26
#define BB_Y  (SH - OB_M - 1 - BB_H)
#define BB_X  TB_X
#define BB_W  TB_W

#define DR_X  TB_X
#define DR_Y  (TB_Y + TB_H + 2)
#define DR_W  TB_W
#define DR_H  (BB_Y - 2 - DR_Y)

#define ROW_H_H  (DR_H * 42 / 100)
#define ROW_H_M  (DR_H * 33 / 100)
#define ROW_H_S  (DR_H - ROW_H_H - ROW_H_M)

#define ROW_Y_H  0
#define ROW_Y_M  (ROW_Y_H + ROW_H_H)
#define ROW_Y_S  (ROW_Y_M + ROW_H_M)

#define SLOT_SIDE_W  34
#define SLOT_CTR_W   104
#define CTR_CX       (SLOT_SIDE_W*3 + SLOT_CTR_W/2)

#define ANIM_STEPS  10
#define ANIM_DELAY  14
#define TICK_H      6

// Boot screen layout
#define PB_X  40
#define PB_Y  130
#define PB_W  240
#define PB_H  5
#define PB_LY 108

// Solar layout
#define DEG2RAD (M_PI / 180.0)
#define OX      (SW / 2)
#define OY      (SH / 2)
#define TILT    0.42f
#define ROTATE_DEG (-30.0f)

enum ScreenMode { SCREEN_CLOCK, SCREEN_WEATHER, SCREEN_SOLAR, SCREEN_SNAKE, SCREEN_DOODLE, SCREEN_SYSTEM };

extern DisplayCompat tft;
extern XPT2046_Touchscreen ts;
extern const char* ssid;
extern const char* password;
extern String apiKey;
extern String city;
extern String temperature;
extern String weatherDesc;
extern const char* ntpServer;
extern const long gmtOffset_sec;

extern ScreenMode currentScreen;
extern unsigned long lastTouchMs;

extern uint16_t C_BG, C_PUR, C_PUR2, C_PUR_DIM, C_WHITE, C_YELLOW, C_GREEN;

void initColours();
int readTouchAction();
bool readTouchPoint(int& x, int& y);
bool touchPressed(int action);
bool buttonPressed(int action);
void resetTouchState();
void navigateScreens(int direction);

void bootSequence();
void shutdownWifiRadio();

void initSprites();
void showClockScreen();
void updateClockScreen();
void renderClockSnapshot();
void fetchWeather();
bool fetchSpaceWeather();

void showWeatherScreen();
void updateWeatherScreen();
void renderWeatherSnapshot();
void freeWeatherCanvas();

void initSolarColours();
void initOrbits();
void showSolarScreen();
void updateSolarScreen();
void renderSolarSnapshot();

void showSnakeScreen();
void updateSnakeScreen();
void renderSnakeSnapshot();

void showDoodleScreen();
void updateDoodleScreen();
void renderDoodleSnapshot();

void showSystemScreen();
void updateSystemScreen();
void renderSystemSnapshot();
