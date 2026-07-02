#include "AppConfig.h"

static unsigned long lastSystemDraw = 0;

String formatBytes(uint32_t bytes) {
  if (bytes >= 1024 * 1024) {
    return String(bytes / (1024.0f * 1024.0f), 1) + " MB";
  }
  return String(bytes / 1024.0f, 1) + " KB";
}

String formatUptime(unsigned long ms) {
  unsigned long total = ms / 1000;
  unsigned long days = total / 86400;
  total %= 86400;
  unsigned long hours = total / 3600;
  total %= 3600;
  unsigned long mins = total / 60;
  unsigned long secs = total % 60;

  char buf[24];
  if (days > 0) {
    sprintf(buf, "%lud %02lu:%02lu:%02lu", days, hours, mins, secs);
  } else {
    sprintf(buf, "%02lu:%02lu:%02lu", hours, mins, secs);
  }
  return String(buf);
}

void drawSystemRow(int y, const char* label, const String& value) {
  tft.setTextFont(1);
  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(C_PUR_DIM, C_BG);
  tft.drawString(label, 18, y);

  tft.setTextDatum(MR_DATUM);
  tft.setTextColor(C_WHITE, C_BG);
  tft.drawString(value, SW - 18, y);
  tft.setTextDatum(TL_DATUM);
}

void drawSystemInfo() {
  uint32_t heapTotal = ESP.getHeapSize();
  uint32_t heapFree = ESP.getFreeHeap();
  uint32_t heapUsed = heapTotal > heapFree ? heapTotal - heapFree : 0;
  uint32_t heapPct = heapTotal > 0 ? (heapUsed * 100UL) / heapTotal : 0;

  tft.fillScreen(C_BG);
  tft.drawRoundRect(3, 3, SW - 6, SH - 6, 10, C_PUR);
  tft.drawRoundRect(5, 5, SW - 10, SH - 10, 9, C_PUR_DIM);

  tft.setTextFont(2);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(C_PUR, C_BG);
  tft.drawString("SYSTEM INFO", SW / 2, 18);
  tft.drawFastHLine(18, 32, SW - 36, C_PUR_DIM);

  int y = 48;
  drawSystemRow(y, "CPU Model", String(ESP.getChipModel())); y += 18;
  drawSystemRow(y, "CPU Frequency", String(ESP.getCpuFreqMHz()) + " MHz"); y += 18;
  drawSystemRow(y, "Chip Revision", String(ESP.getChipRevision())); y += 18;
  drawSystemRow(y, "Flash Size", formatBytes(ESP.getFlashChipSize())); y += 18;
  drawSystemRow(y, "Flash Speed", String(ESP.getFlashChipSpeed() / 1000000) + " MHz"); y += 18;
  drawSystemRow(y, "Free RAM", formatBytes(heapFree)); y += 18;
  drawSystemRow(y, "Minimum Free RAM", formatBytes(ESP.getMinFreeHeap())); y += 18;
  drawSystemRow(y, "Heap Usage", formatBytes(heapUsed) + " / " + String(heapPct) + "%"); y += 18;
  drawSystemRow(y, "Uptime", formatUptime(millis()));

  tft.setTextFont(1);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(C_PUR_DIM, C_BG);
  tft.drawString("UP/DOWN FOR PAGES", SW / 2, SH - 14);
  tft.setTextDatum(TL_DATUM);
}

void showSystemScreen() {
  currentScreen = SCREEN_SYSTEM;
  lastSystemDraw = 0;
  drawSystemInfo();
}

void renderSystemSnapshot() {
  drawSystemInfo();
}

void updateSystemScreen() {
  unsigned long now = millis();
  if (now - lastSystemDraw >= 1000) {
    lastSystemDraw = now;
    drawSystemInfo();
  }
}
