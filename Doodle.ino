#include "AppConfig.h"

static bool     doodlePenDown = false;
static int      doodleLastX   = 0;
static int      doodleLastY   = 0;
static uint16_t doodleInk;
static uint16_t doodleBg;

void drawDoodleFrame() {
  tft.fillScreen(doodleBg);                           // solid black canvas
  // Thin header bar
  tft.fillRect(0, 0, SW, 22, C_BG);
  tft.drawFastHLine(0, 22, SW, C_PUR_DIM);            // separator line
  tft.setTextDatum(ML_DATUM);
  tft.setTextFont(2);
  tft.setTextColor(C_PUR, C_BG);
  tft.drawString("DOODLE", 8, 11);
  tft.setTextDatum(MR_DATUM);
  tft.setTextFont(1);
  tft.setTextColor(C_PUR_DIM, C_BG);
  tft.drawString("SELECT: CLEAR", SW - 8, 11);
  tft.setTextDatum(TL_DATUM);
}

void showDoodleScreen() {
  currentScreen = SCREEN_DOODLE;
  doodleBg  = tft.color565(0, 0, 0);       // black canvas
  doodleInk = tft.color565(255, 255, 255); // white pen
  doodlePenDown = false;
  resetTouchState();
  drawDoodleFrame();
}

void renderDoodleSnapshot() {
  // Snapshot just shows the header; canvas content is ephemeral
  drawDoodleFrame();
}

void updateDoodleScreen() {
  if (currentScreen != SCREEN_DOODLE) return;

  if (buttonPressed(TOUCH_SELECT)) {
    // Clear: redraw the blank canvas
    tft.fillRect(0, 23, SW, SH - 23, doodleBg);
    doodlePenDown = false;
    return;
  }

  int x, y;
  if (!readTouchPoint(x, y) || y < 24) {
    // Finger lifted or in header — lift pen cleanly
    doodlePenDown = false;
    return;
  }

  if (doodlePenDown) {
    // Smooth anti-aliased-style line: draw three parallel lines
    // for a slightly thicker, smoother stroke without visible dots
    tft.drawLine(doodleLastX,     doodleLastY,     x,     y,     doodleInk);
    tft.drawLine(doodleLastX + 1, doodleLastY,     x + 1, y,     doodleInk);
    tft.drawLine(doodleLastX,     doodleLastY + 1, x,     y + 1, doodleInk);
  }
  // No dot placed on first touch — pen-down starts drawing only on move

  doodleLastX   = x;
  doodleLastY   = y;
  doodlePenDown = true;
}
