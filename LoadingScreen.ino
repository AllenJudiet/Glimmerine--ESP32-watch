#include "AppConfig.h"
//loadingscreen.h
void drawBootLabel(const char* msg, uint16_t col) {
  tft.fillRect(PB_X, PB_LY, PB_W, 14, C_BG);
  tft.setTextFont(2);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(col, C_BG);
  tft.drawString(msg, SW / 2, PB_LY + 7);
}

void drawBootDetail(const String& msg) {
  tft.fillRect(12, PB_Y + 16, SW - 24, 14, C_BG);
  tft.setTextFont(1);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(C_PUR_DIM, C_BG);
  tft.drawString(msg, SW / 2, PB_Y + 23);
  tft.setTextDatum(TL_DATUM);
}

void drawBootProgress(int pct) {
  tft.drawRect(PB_X, PB_Y, PB_W, PB_H, C_PUR);
  int f = (PB_W - 2) * pct / 100;
  tft.fillRect(PB_X + 1, PB_Y + 1, f, PB_H - 2, C_PUR);
  tft.fillRect(PB_X + 1 + f, PB_Y + 1, (PB_W - 2) - f, PB_H - 2, C_BG);
}

void drawBootDots(int frame) {
  for (int i = 0; i < 3; i++) {
    tft.fillCircle(SW / 2 + 68 + i * 10, PB_LY + 7, 2,
                   i == frame % 3 ? C_PUR : C_PUR_DIM);
  }
}

const char* wifiStatusText(int status) {
  static char buf[32];
  if (status == WL_IDLE_STATUS) return "IDLE";
  if (status == WL_NO_SSID_AVAIL) return "SSID NOT FOUND";
  if (status == WL_SCAN_COMPLETED) return "SCAN COMPLETED";
  if (status == WL_CONNECT_FAILED) return "CONNECT FAILED";
  if (status == WL_CONNECTION_LOST) return "CONNECTION LOST";
  if (status == WL_DISCONNECTED) return "DISCONNECTED";
  if (status == WL_CONNECTED) return "CONNECTED";
  sprintf(buf, "FAILED (%d)", status);
  return buf;
}

void resetBootWifiSta() {
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  esp_wifi_set_ps(WIFI_PS_NONE);
  esp_wifi_set_max_tx_power(78);
}

bool waitBootWifi(const char* label, int startPct, int endPct, int& dotF, int maxTries) {
  drawBootLabel(label, C_PUR);
  for (int tries = 0; maxTries < 0 || tries < maxTries; tries++) {
    int status = WiFi.status();
    if (status == WL_CONNECTED) {
      Serial.print("WiFi connected, IP=");
      Serial.print(WiFi.localIP());
      Serial.print(", RSSI=");
      Serial.println(WiFi.RSSI());
      drawBootDetail(String("IP ") + WiFi.localIP().toString() +
                     "  RSSI " + String(WiFi.RSSI()));
      return true;
    }

    if (tries % 5 == 0) {
      Serial.print(label);
      Serial.print(" status=");
      Serial.print(status);
      Serial.print(" ");
      Serial.println(wifiStatusText(status));
      drawBootDetail(String(wifiStatusText(status)) + " (" + String(status) + ")");
    }

    int progressLimit = maxTries > 0 ? maxTries : 150;
    int progressTries = min(tries, progressLimit);
    drawBootProgress(startPct + (endPct - startPct) * progressTries / progressLimit);
    drawBootDots(dotF++);
    delay(250);
  }
  return false;
}

bool connectBootWifi(int& dotF) {
  resetBootWifiSta();

  Serial.println("Connecting...");
  drawBootLabel("CONNECTING TO WIFI", C_PUR);
  drawBootDetail(String("SSID: ") + ssid);
  WiFi.begin(ssid, password);
  
  return waitBootWifi("CONNECTING TO WIFI", 0, 55, dotF, 40);
}

void shutdownWifiRadio() {
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  esp_wifi_stop();
  Serial.println("WiFi radio off");
}

void bootSequence() {
  tft.fillScreen(C_BG);
  tft.setTextFont(4);
  tft.setTextColor(C_PUR, C_BG);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("Glimmerine V0", SW / 2, 52);
  tft.setTextFont(2);
  tft.setTextColor(C_PUR_DIM, C_BG);
  tft.drawString("INITIALISING SYSTEMS", SW / 2, 78);
  tft.setTextDatum(TL_DATUM);

  drawBootLabel("CONNECTING TO WIFI", C_PUR);
  drawBootProgress(0);

  int dotF = 0;
  bool wifiOk = connectBootWifi(dotF);

  if (wifiOk && WiFi.status() == WL_CONNECTED) {
    drawBootLabel("SYNCING NTP TIME", C_PUR);
    drawBootProgress(55);

    configTime(gmtOffset_sec, 0, ntpServer);
    struct tm t;
    int ntpT = 0;
    while (!getLocalTime(&t)) {
      delay(300);
      ntpT++;
      drawBootProgress(55 + min(25, ntpT * 3));
      drawBootDots(dotF++);
      if (ntpT > 20) {
        drawBootLabel("TIME SYNC FAILED", C_YELLOW);
        delay(600);
        break;
      }
    }
  } else {
    drawBootLabel("WIFI SKIPPED", C_YELLOW);
    drawBootDetail("Starting offline");
    drawBootProgress(80);
    delay(600);
  }

  drawBootProgress(80);
  if (wifiOk && WiFi.status() == WL_CONNECTED) {
    drawBootLabel("FETCHING WEATHER", C_PUR);
    drawBootProgress(85);
    fetchWeather();
    drawBootProgress(95);
  }
  shutdownWifiRadio();
  drawBootProgress(100);
  drawBootLabel("SYSTEMS ONLINE", C_GREEN);
  delay(600);
}
