#include "AppConfig.h"
//clock.h
TFT_eSprite sTop  = TFT_eSprite(&tft);
TFT_eSprite sDrum = TFT_eSprite(&tft);
TFT_eSprite sBot  = TFT_eSprite(&tft);

String temperature = "--";
String weatherDesc = "";
int lastH = -1, lastM = -1, lastS = -1;
int prevHour = -1, prevMin = -1, prevSec = -1;
int lastWeatherSec = -WEATHER_INTERVAL_SEC;

void drawRRect(int x, int y, int w, int h, int r, uint16_t c) {
  tft.drawRoundRect(x, y, w, h, r, c);
}

void drawBracket(int x, int y, int dir, uint16_t col) {
  int len = 12;
  int hx = (dir == 1 || dir == 3) ? x - len : x;
  int vx = (dir == 1 || dir == 3) ? x       : x + len;
  int vy = (dir == 2 || dir == 3) ? y - len : y;
  tft.drawFastHLine(hx, y, len + 1, col);
  tft.drawFastVLine(vx, vy, len + 1, col);
}

void drawDrumDotTrio(int cx, int cy, uint16_t col) {
  for (int i = -1; i <= 1; i++) sDrum.fillCircle(cx + i * 7, cy, 2, col);
}

void drawBotDotTrio(int cx, int cy, uint16_t col) {
  for (int i = -1; i <= 1; i++) sBot.fillCircle(cx + i * 7, cy, 2, col);
}

void drawTopCloudIcon(int cx, int cy, uint16_t col) {
  sTop.fillCircle(cx - 5, cy + 1, 4, col);
  sTop.fillCircle(cx,     cy - 2, 5, col);
  sTop.fillCircle(cx + 5, cy + 1, 4, col);
  sTop.fillRect(cx - 9, cy + 1, 19, 5, col);
  sTop.fillCircle(cx - 5, cy + 1, 2, C_BG);
  sTop.fillCircle(cx,     cy - 2, 3, C_BG);
  sTop.fillCircle(cx + 5, cy + 1, 2, C_BG);
  sTop.fillRect(cx - 7, cy + 2, 15, 3, C_BG);
}

void drawTopPinIcon(int cx, int cy, uint16_t col) {
  sTop.fillCircle(cx, cy - 2, 5, col);
  sTop.fillTriangle(cx - 4, cy - 1, cx + 4, cy - 1, cx, cy + 7, col);
  sTop.fillCircle(cx, cy - 2, 2, C_BG);
}

int slotCX(int off) {
  if (off < 0) return CTR_CX - SLOT_CTR_W / 2 + off * SLOT_SIDE_W + SLOT_SIDE_W / 2;
  if (off == 0) return CTR_CX;
  return CTR_CX + SLOT_CTR_W / 2 + (off - 1) * SLOT_SIDE_W + SLOT_SIDE_W / 2;
}

void drawDrumRow(int val, int modulo, int rowY, int rh, int shift, int bigFont) {
  int railY = rowY + rh - 10;
  sDrum.drawFastHLine(10, railY, DR_W - 20, C_PUR_DIM);
  sDrum.drawCircle(10, railY, 3, C_PUR_DIM);
  sDrum.drawCircle(DR_W - 10, railY, 3, C_PUR_DIM);

  for (int off = -4; off <= 4; off++) {
    int cx = slotCX(off) + shift;
    if (cx < -30 || cx > DR_W + 30) continue;

    int slotVal = ((val + off) % modulo + modulo) % modulo;
    char buf[4];
    sprintf(buf, "%02d", slotVal);

    if (off == 0) {
      sDrum.setTextFont(bigFont);
      sDrum.setTextColor(C_PUR, C_BG);
      sDrum.setTextDatum(MC_DATUM);
      sDrum.drawString(buf, cx, rowY + rh * 45 / 100);
      sDrum.drawFastVLine(cx, railY - TICK_H, TICK_H, C_PUR);
    } else {
      uint16_t col = (abs(off) == 1) ? C_PUR2 : C_PUR_DIM;
      sDrum.setTextFont(1);
      sDrum.setTextColor(col, C_BG);
      sDrum.setTextDatum(MC_DATUM);
      sDrum.drawString(buf, cx, railY - TICK_H - 5);
      sDrum.drawFastVLine(cx, railY - 4, 4, col);
    }
  }
  sDrum.setTextDatum(TL_DATUM);
}

void composeDrum(int h, int m, int s, int shiftH, int shiftM, int shiftS) {
  sDrum.fillSprite(C_BG);
  drawDrumDotTrio(DR_W / 2, 4, C_PUR_DIM);
  sDrum.drawFastHLine(20, ROW_Y_M, DR_W - 40, C_PUR_DIM);
  sDrum.drawFastHLine(20, ROW_Y_S, DR_W - 40, C_PUR_DIM);
  drawDrumRow(h, 24, ROW_Y_H + 2, ROW_H_H - 4, shiftH, 6);
  drawDrumRow(m, 60, ROW_Y_M + 2, ROW_H_M - 4, shiftM, 4);
  drawDrumRow(s, 60, ROW_Y_S + 2, ROW_H_S - 4, shiftS, 2);
}

void renderDrumStatic(int h, int m, int s) {
  composeDrum(h, m, s, 0, 0, 0);
  sDrum.pushSprite(DR_X, DR_Y);
}

void animateDrum(int newH, int newM, int newS, int oldH, int oldM, int oldS) {
  bool chH = (newH != oldH);
  bool chM = (newM != oldM);

  for (int step = 0; step <= ANIM_STEPS; step++) {
    int shift = SLOT_SIDE_W * (ANIM_STEPS - step) / ANIM_STEPS;
    composeDrum(newH, newM, newS, chH ? shift : 0, chM ? shift : 0, shift);
    sDrum.pushSprite(DR_X, DR_Y);
    delay(ANIM_DELAY);
  }
}

void renderTop() {
  sTop.fillSprite(C_BG);
  sTop.setTextFont(2);
  sTop.setTextColor(C_PUR, C_BG);
  sTop.setTextDatum(MC_DATUM);
  sTop.drawString("Glimmerine V0", TB_W / 2, TB_H / 2 + 1);
  sTop.setTextDatum(TL_DATUM);
  sTop.pushSprite(TB_X, TB_Y);
}

void renderBot(struct tm* t) {
  sBot.fillSprite(C_BG);
  drawBotDotTrio(BB_W / 2, 3, C_PUR_DIM);

  int lineW = 50, lineY = 3;
  sBot.drawFastHLine(BB_W / 2 - 30 - lineW, lineY, lineW, C_PUR_DIM);
  sBot.drawFastHLine(BB_W / 2 + 30, lineY, lineW, C_PUR_DIM);
  sBot.drawLine(BB_W / 2 - 30 - lineW, lineY, BB_W / 2 - 30 - lineW - 6, lineY + 4, C_PUR_DIM);
  sBot.drawLine(BB_W / 2 + 30 + lineW, lineY, BB_W / 2 + 30 + lineW + 6, lineY + 4, C_PUR_DIM);

  char dateBuf[20];
  sprintf(dateBuf, "%02d : %02d : %04d", t->tm_mday, t->tm_mon + 1, t->tm_year + 1900);
  sBot.setTextFont(2);
  sBot.setTextColor(C_PUR, C_BG);
  sBot.setTextDatum(MC_DATUM);
  sBot.drawString(dateBuf, BB_W / 2, BB_H / 2 + 5);
  sBot.setTextDatum(TL_DATUM);
  sBot.pushSprite(BB_X, BB_Y);
}

void fetchWeather() {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  String url = "http://api.openweathermap.org/data/2.5/weather?q=" +
               city + "&appid=" + apiKey + "&units=metric";
  http.setConnectTimeout(3000);  // TCP connect timeout
  http.setTimeout(3000);         // HTTP response timeout
  http.begin(url);
  int code = http.GET();
  if (code == 200) {
    StaticJsonDocument<1024> doc;
    DeserializationError err = deserializeJson(doc, http.getString());
    if (!err) {
      temperature = String((int)doc["main"]["temp"]);
      weatherDesc = String((const char*)doc["weather"][0]["main"]);
    }
  }
  http.end();
}

void initSprites() {
  sTop.createSprite(TB_W, TB_H);
  sTop.setColorDepth(16);
  sDrum.createSprite(DR_W, DR_H);
  sDrum.setColorDepth(16);
  sBot.createSprite(BB_W, BB_H);
  sBot.setColorDepth(16);
}

void drawClockFrame() {
  tft.fillScreen(C_BG);
  drawRRect(OB_M, OB_M, SW - 2 * OB_M, SH - 2 * OB_M, OB_R, C_PUR);
  int bx0 = OB_M + 5, by0 = OB_M + 5;
  int bx1 = SW - OB_M - 6, by1 = SH - OB_M - 6;
  drawBracket(bx0, by0, 0, C_PUR2);
  drawBracket(bx1, by0, 1, C_PUR2);
  drawBracket(bx0, by1, 2, C_PUR2);
  drawBracket(bx1, by1, 3, C_PUR2);
}

void showClockScreen() {
  currentScreen = SCREEN_CLOCK;
  drawClockFrame();
  renderTop();

  struct tm t;
  if (getLocalTime(&t)) {
    renderDrumStatic(t.tm_hour, t.tm_min, t.tm_sec);
    renderBot(&t);
    lastH = prevHour = t.tm_hour;
    lastM = prevMin  = t.tm_min;
    lastS = prevSec  = t.tm_sec;
  }
}

void renderClockSnapshot() {
  drawClockFrame();
  renderTop();

  struct tm t;
  if (getLocalTime(&t)) {
    renderDrumStatic(t.tm_hour, t.tm_min, t.tm_sec);
    renderBot(&t);
  } else {
    renderDrumStatic(max(0, lastH), max(0, lastM), max(0, lastS));
  }
}

void updateClockScreen() {
  struct tm t;
  if (!getLocalTime(&t)) {
    delay(100);
    return;
  }

  int h = t.tm_hour, m = t.tm_min, s = t.tm_sec;

  if (s != lastS) {
    bool dateChanged = (m != lastM || h != lastH);
    animateDrum(h, m, s, prevHour, prevMin, prevSec);

    prevHour = lastH = h;
    prevMin  = lastM = m;
    prevSec  = lastS = s;

    if (dateChanged) renderBot(&t);
  }

  if (lastWeatherSec < 0) {
    lastWeatherSec = h * 3600 + m * 60 + s;
  }
}
