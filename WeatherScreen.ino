#include "AppConfig.h"

// ── State ────────────────────────────────────────────────────────────────────
static unsigned long lastWeatherAnimMs = 0;
static int  weatherAnimFrame = 0;
static bool weatherNeedsFullDraw = true;   // draw background + cloud on first frame
static bool weatherWasSunny = false;       // detect condition change

// Inline RGB565 helper (no canvas, no color565 issue)
static inline uint16_t wcolor(uint8_t r, uint8_t g, uint8_t b) {
  return ((uint16_t)(r & 0xF8) << 8) |
         ((uint16_t)(g & 0xFC) << 3) |
         (b >> 3);
}

// Precomputed colours (set once per full-draw)
static uint16_t W_BG, W_CLOUD, W_RAIN, W_TEXT, W_DARK;

// ── Condition helpers ────────────────────────────────────────────────────────
bool weatherIsRainy() {
  String w = weatherDesc; w.toLowerCase();
  return w.indexOf("rain") >= 0 || w.indexOf("drizzle") >= 0 ||
         w.indexOf("thunderstorm") >= 0;
}
bool weatherIsSunny() {
  String w = weatherDesc; w.toLowerCase();
  return w.indexOf("clear") >= 0 || w.indexOf("sun") >= 0;
}

#include <Fonts/FreeSans9pt7b.h>

// freeWeatherCanvas kept as a no-op so AppConfig.h / main.ino still compile
void freeWeatherCanvas() {}

static void drawVectorStringCentred(const char* msg, int cx, int cy, uint16_t col) {
  tft.setFont(&FreeSans9pt7b);
  tft.setTextColor(col, 0);
  
  int16_t x1, y1;
  uint16_t w, h;
  tft.getTextBounds(msg, 0, 0, &x1, &y1, &w, &h);
  
  int tx = cx - (int)w / 2;
  int ty = cy - (y1 + (int)h / 2);
  
  tft.setCursor(tx, ty);
  tft.print(msg);
  
  tft.setFont(NULL); // reset to default font
}

// ── Cloud shape ──────────────────────────────────────────────────────────────
static void drawCloud(int cx, int cy, uint16_t col) {
  tft.fillCircle(cx - 30, cy + 6,  22, col);
  tft.fillCircle(cx -  4, cy - 8,  30, col);
  tft.fillCircle(cx + 28, cy + 4,  23, col);
  tft.fillRoundRect(cx - 54, cy + 6, 110, 30, 12, col);
}

// ── Temperature + condition inside the cloud ─────────────────────────────────
static void drawCloudLabel(int cx, int cy, uint16_t col, uint16_t bg) {
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(col, bg);
  tft.setTextFont(4);   // tallest available font
  tft.drawString(temperature + "C", cx, cy - 4);
  tft.setTextFont(1);
  String cond = weatherDesc; cond.toUpperCase();
  tft.drawString(cond, cx, cy + 18);
  tft.setTextDatum(TL_DATUM);
}

// ── Full background + static elements ───────────────────────────────────────
static void drawWeatherBackground(bool sunny) {
  W_BG    = wcolor(  0,   0,   0);
  W_CLOUD = wcolor(120,  80, 175);
  W_RAIN  = wcolor( 80, 150, 255);
  W_TEXT  = sunny ? wcolor(255, 230, 120) : wcolor(210, 210, 255);
  W_DARK  = wcolor( 20,   0,  40);

  tft.fillScreen(W_BG);
  tft.drawRoundRect(3, 3, SW - 6, SH - 6, 10, W_CLOUD);

  if (sunny) {
    // Static sun disc behind the cloud
    tft.fillCircle(SW / 2, 72, 33, wcolor(255, 215, 60));
    tft.fillCircle(SW / 2 - 9, 65, 3, W_BG);
    tft.fillCircle(SW / 2 + 9, 65, 3, W_BG);
    tft.drawLine(SW/2-10, 80, SW/2-5,  85, W_BG);
    tft.drawLine(SW/2-5,  85, SW/2+5,  85, W_BG);
    tft.drawLine(SW/2+5,  85, SW/2+10, 80, W_BG);
  }

  // Cloud on top
  drawCloud(SW / 2, 72, W_CLOUD);
  drawCloudLabel(SW / 2, 68, W_DARK, W_CLOUD);

  // Static message text at bottom
  if (sunny) {
    drawVectorStringCentred("sun is smiling bright", SW / 2, 196, W_TEXT);
  } else {
    drawVectorStringCentred("the rain is speaking quietly", SW / 2, 175, W_TEXT);
    drawVectorStringCentred("you can sleep now", SW / 2, 198, W_TEXT);
  }
}

// ── Rain animation: only repaint the rain strip each frame ──────────────────
// Rain zone: from below the cloud (y=106) to above the text (y=160)
#define RAIN_Y1  106
#define RAIN_Y2  160

static void drawRainFrame(int frame) {
  // Erase old strip
  tft.fillRect(4, RAIN_Y1, SW - 8, RAIN_Y2 - RAIN_Y1, W_BG);

  // Draw streaks
  for (int i = 0; i < 24; i++) {
    int bx = 10 + i * 12 + (i % 3) * 4;
    int by = RAIN_Y1 + ((frame * 5 + i * 19) % (RAIN_Y2 - RAIN_Y1 + 13));
    int ey = by + 13;
    if (ey > RAIN_Y2) ey = RAIN_Y2;
    if (by >= RAIN_Y2) continue;
    tft.drawLine(bx,     by, bx - 4, ey, W_RAIN);
    tft.drawLine(bx + 1, by, bx - 3, ey, W_RAIN);
  }
}

// ── Sunny animation: smooth pulsing glow rings only ─────────────────────────
// Glow zone: around the sun (before the cloud is drawn on top of it)
// We redraw the sun + glow area, then re-overlay the cloud each frame
#define SUN_CX  (SW / 2)
#define SUN_CY  72
#define GLOW_ZONE_X  (SUN_CX - 72)
#define GLOW_ZONE_Y  (SUN_CY - 72)
#define GLOW_ZONE_W  144
#define GLOW_ZONE_H  144

static void drawSunFrame(int frame) {
  uint16_t glowOuter = wcolor(100,  30, 180);
  uint16_t glowMid   = wcolor(160,  70, 220);
  uint16_t sunCol    = wcolor(255, 215,  60);
  uint16_t rayCol    = wcolor(255, 240, 100);

  float t     = (frame % 32) * (2.0f * 3.14159f / 32.0f);
  int   pulse = (int)(5.0f + 4.0f * sinf(t));

  // Erase sun zone
  tft.fillRect(GLOW_ZONE_X, GLOW_ZONE_Y, GLOW_ZONE_W, GLOW_ZONE_H, W_BG);

  // Glow rings
  for (int r = 58 + pulse; r > 38; r -= 10)
    tft.drawCircle(SUN_CX, SUN_CY, r, (r > 50 + pulse/2) ? glowOuter : glowMid);

  // Rays
  for (int a = 0; a < 360; a += 30) {
    float rad = (float)a * DEG2RAD + t * 0.12f;
    int rx1 = SUN_CX + (int)(cosf(rad) * 40);
    int ry1 = SUN_CY + (int)(sinf(rad) * 40);
    int rx2 = SUN_CX + (int)(cosf(rad) * (56 + pulse));
    int ry2 = SUN_CY + (int)(sinf(rad) * (56 + pulse));
    tft.drawLine(rx1,     ry1, rx2,     ry2, rayCol);
    tft.drawLine(rx1 + 1, ry1, rx2 + 1, ry2, sunCol);
  }

  // Re-draw sun disc
  tft.fillCircle(SUN_CX, SUN_CY, 33, sunCol);
  tft.fillCircle(SUN_CX - 9, SUN_CY - 7, 3, W_BG);
  tft.fillCircle(SUN_CX + 9, SUN_CY - 7, 3, W_BG);
  tft.drawLine(SUN_CX-10, SUN_CY+8, SUN_CX-5,  SUN_CY+13, W_BG);
  tft.drawLine(SUN_CX-5,  SUN_CY+13,SUN_CX+5,  SUN_CY+13, W_BG);
  tft.drawLine(SUN_CX+5,  SUN_CY+13,SUN_CX+10, SUN_CY+8,  W_BG);

  // Re-overlay cloud so it always appears in front of sun
  drawCloud(SUN_CX, SUN_CY, W_CLOUD);
  drawCloudLabel(SUN_CX, SUN_CY - 4, W_DARK, W_CLOUD);
}

// ── Screen lifecycle ─────────────────────────────────────────────────────────
void showWeatherScreen() {
  currentScreen       = SCREEN_WEATHER;
  lastWeatherAnimMs   = 0;
  weatherAnimFrame    = 0;
  weatherNeedsFullDraw = true;
  bool sunny = weatherIsSunny();
  weatherWasSunny     = sunny;
  drawWeatherBackground(sunny);
  if (!sunny) drawRainFrame(0);
}

void renderWeatherSnapshot() {
  bool sunny = weatherIsSunny();
  drawWeatherBackground(sunny);
  if (!sunny) drawRainFrame(weatherAnimFrame);
}

void updateWeatherScreen() {
  unsigned long now = millis();
  if (now - lastWeatherAnimMs < 50) return;   // ~20 fps
  lastWeatherAnimMs = now;
  weatherAnimFrame++;

  bool sunny = weatherIsSunny();

  // Re-draw everything if condition changed since last full draw
  if (sunny != weatherWasSunny || weatherNeedsFullDraw) {
    weatherWasSunny     = sunny;
    weatherNeedsFullDraw = false;
    drawWeatherBackground(sunny);
  }

  if (sunny) drawSunFrame(weatherAnimFrame);
  else       drawRainFrame(weatherAnimFrame);
}
