#include "AppConfig.h"
//solarsytem.h
uint16_t C_ORBIT, C_ORBIT_GLOW, C_SUN_CORE, C_SUN_GLOW;
const int PLANET_COUNT = 9;
uint16_t PCOL[PLANET_COUNT];
String geomagneticState = "QUIET";
String solarWindSpeed = "-- km/s";
String flareCountToday = "-- today";
unsigned long lastSpaceWeatherFetchMs = 0;
unsigned long lastSpaceWeatherAttemptMs = 0;
int selectedPlanet = -1;
bool solarTouchWasDown = false;
int lastPlanetX[PLANET_COUNT];
int lastPlanetY[PLANET_COUNT];

const char* PLANET_NAMES[PLANET_COUNT] = {
  "Mercury", "Venus", "Earth", "Mars", "Jupiter", "Saturn", "Uranus", "Neptune", "Pluto"
};

const float PLANET_DAY_HOURS[PLANET_COUNT] = {
  1407.5f, 5832.5f, 24.0f, 24.623f, 9.925f, 10.656f, 17.24f, 16.11f, 153.3f
};

OrbitalElements P[PLANET_COUNT] = {
  { 0.38709843,0.20563661, 7.00559432,252.25166724, 77.45771895, 48.33961819,
    0.0,0.00002123,-0.00590158,149472.67486623,0.15940013,-0.12214182, 28, 0, 2 },
  { 0.72332102,0.00676399, 3.39777545,181.97970850,131.76755713, 76.67261496,
   -0.00000026,-0.00005107,0.00043494,58517.81538729,0.05679648,-0.27274174, 40, 0, 2 },
  { 1.00000018,0.01673163,-0.00054346,100.46691572,102.93005885, -5.11260389,
   -0.00000003,-0.00003661,-0.01337178,35999.37306329,0.31795260,-0.24123856, 52, 0, 2 },
  { 1.52371243,0.09336511, 1.85181869, -4.56813164,-23.91744784, 49.71320984,
    0.00000097,0.00009149,-0.00724757,19140.29934243,0.45223625,-0.26852431, 64, 0, 2 },
  { 5.20248019,0.04853590, 1.29861416, 34.33479152, 14.27495244,100.29282654,
   -0.00002864,0.00018026,-0.00322699,3034.90371757,0.18199196,0.13024619, 82, 0, 4 },
  { 9.54149883,0.05550825, 2.49424102, 50.07571329, 92.86136063,113.63998702,
   -0.00003065,-0.00032044,0.00451969,1222.11494724,0.54179478,-0.25015002, 98, 0, 4 },
  { 19.18797948,0.04685740,0.77298127,314.20276625,172.43404441, 73.96250215,
   -0.00020455,-0.00001550,-0.00180155,428.49512595,0.09266985,0.05739699, 113, 0, 3 },
  { 30.06952752,0.00895439,1.77005520,304.22289287, 46.68158724,131.78635853,
    0.00006447,0.00000818,0.00022400,218.46515314,0.01009938,-0.00606302, 126, 0, 3 },
  { 39.48686035,0.24885238,17.14104260,238.96535011,224.09702598,110.30167986,
    0.00449751,0.00006016,0.00000501,145.18042903,-0.00968827,-0.00809981, 145, 0, 2 }
};

void initSolarColours() {
  C_ORBIT      = tft.color565(55, 0, 90);
  C_ORBIT_GLOW = tft.color565(80, 0, 130);
  C_SUN_CORE   = tft.color565(160, 30, 220);
  C_SUN_GLOW   = tft.color565(80, 0, 120);

  PCOL[0] = tft.color565(90, 20, 140);
  PCOL[1] = tft.color565(130, 40, 190);
  PCOL[2] = tft.color565(80, 60, 200);
  PCOL[3] = tft.color565(100, 10, 160);
  PCOL[4] = tft.color565(140, 50, 200);
  PCOL[5] = tft.color565(120, 40, 180);
  PCOL[6] = tft.color565(70, 80, 210);
  PCOL[7] = tft.color565(60, 30, 180);
  PCOL[8] = tft.color565(140, 100, 220);
}

void initOrbits() {
  for (int i = 0; i < PLANET_COUNT; i++) P[i].orb_b = P[i].orb_a * TILT;
}

double solveKepler(double M_deg, double ecc) {
  double M = fmod(M_deg, 360.0);
  if (M < 0) M += 360.0;
  double E = M + (ecc * 180.0 / M_PI) * sin(M * DEG2RAD);
  for (int i = 0; i < 50; i++) {
    double dE = (M - (E - (ecc * 180.0 / M_PI) * sin(E * DEG2RAD))) /
                (1.0 - ecc * cos(E * DEG2RAD));
    E += dE;
    if (fabs(dE) < 1e-6) break;
  }
  return E;
}

double planetAngle(int idx, double T) {
  const OrbitalElements& p = P[idx];
  double e  = p.e  + p.de  * T;
  double L  = p.L  + p.dL  * T;
  double lp = p.lp + p.dlp * T;
  double M = fmod(L - lp, 360.0);
  if (M < 0) M += 360.0;
  double E  = solveKepler(M, e);
  double nu = 2.0 * atan2(sqrt(1 + e) * sin(E * DEG2RAD / 2.0),
                          sqrt(1 - e) * cos(E * DEG2RAD / 2.0));
  return nu + fmod(lp, 360.0) * DEG2RAD;
}

void angleToScreen(float orb_a, float orb_b, double angle, int& sx, int& sy) {
  double rot = ROTATE_DEG * DEG2RAD;
  float ex = orb_a * cos(angle);
  float ey = orb_b * sin(angle);
  float rx = ex * cos(rot) - ey * sin(rot);
  float ry = ex * sin(rot) + ey * cos(rot);
  sx = OX + (int)(rx + 0.5f);
  sy = OY + (int)(ry + 0.5f);
}

void drawOrbit(float orb_a, float orb_b, uint16_t col) {
  double rot = ROTATE_DEG * DEG2RAD;
  const int STEPS = 100;
  int px = 0, py = 0;
  for (int i = 0; i <= STEPS; i++) {
    double angle = (double)i / STEPS * 2.0 * M_PI;
    float ex = orb_a * cos(angle);
    float ey = orb_b * sin(angle);
    float rx = ex * cos(rot) - ey * sin(rot);
    float ry = ex * sin(rot) + ey * cos(rot);
    int sx = OX + (int)(rx + 0.5f);
    int sy = OY + (int)(ry + 0.5f);
    if (i > 0 && sx >= 0 && sx < SW && sy >= 0 && sy < SH &&
        px >= 0 && px < SW && py >= 0 && py < SH) {
      tft.drawLine(px, py, sx, sy, col);
    }
    px = sx;
    py = sy;
  }
}

void drawSaturnRings(int cx, int cy, int pr, uint16_t col) {
  float ra1 = pr + 4, rb1 = (pr + 4) * TILT * 0.8f;
  float ra2 = pr + 7, rb2 = (pr + 7) * TILT * 0.8f;
  double rot = ROTATE_DEG * DEG2RAD;
  for (int ring = 0; ring < 2; ring++) {
    float ra = (ring == 0) ? ra1 : ra2;
    float rb = (ring == 0) ? rb1 : rb2;
    const int S = 60;
    int ppx = 0, ppy = 0;
    for (int i = 0; i <= S; i++) {
      double a = (double)i / S * 2.0 * M_PI;
      float ex = ra * cos(a), ey = rb * sin(a);
      float rx = ex * cos(rot) - ey * sin(rot);
      float ry = ex * sin(rot) + ey * cos(rot);
      int sx = cx + (int)(rx + 0.5f);
      int sy = cy + (int)(ry + 0.5f);
      if (i > 0) tft.drawLine(ppx, ppy, sx, sy, col);
      ppx = sx;
      ppy = sy;
    }
  }
}

void drawJupiterBands(int cx, int cy, int pr) {
  uint16_t band = tft.color565(160, 60, 220);
  for (int dy = -1; dy <= 1; dy++) tft.drawFastHLine(cx - pr, cy + dy, pr * 2, band);
}

void drawSun() {
  tft.fillCircle(OX, OY, 8, C_SUN_GLOW);
  tft.fillCircle(OX, OY, 6, tft.color565(120, 20, 180));
  tft.fillCircle(OX, OY, 4, C_SUN_CORE);
}

void drawSolarFrame() {
  tft.fillScreen(C_BG);
  tft.drawRoundRect(3, 3, SW - 6, SH - 6, 10, C_PUR);
  tft.drawRoundRect(5, 5, SW - 10, SH - 10, 9, C_PUR_DIM);
}

String geomagneticLabel(float kp) {
  if (kp < 4.0f) return "QUIET";
  if (kp < 5.0f) return "UNSETTLED";
  if (kp < 6.0f) return "STORM G1";
  if (kp < 7.0f) return "STORM G2";
  if (kp < 8.0f) return "STORM G3";
  if (kp < 9.0f) return "STORM G4";
  return "STORM G5";
}

bool fetchPayload(const char* url, String& payload) {
  HTTPClient http;
  http.setTimeout(2500);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  if (!http.begin(url)) {
    Serial.printf("NOAA begin failed: %s\n", url);
    return false;
  }
  int status = http.GET();
  if (status != 200) {
    Serial.printf("NOAA HTTP error %d: %s\n", status, url);
    http.end();
    return false;
  }
  payload = http.getString();
  Serial.printf("NOAA received %d bytes: %s\n", payload.length(), url);
  http.end();
  return true;
}

bool fetchSpaceWeather() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("NOAA skipped: WiFi disconnected");
    return false;
  }

  bool windOk = false;
  bool flaresOk = false;

  {
    // API returns an array of objects: [{"time_tag":"...","Kp":2.33,...}, ...]
    String payload;
    DynamicJsonDocument doc(8000);
    if (fetchPayload("https://services.swpc.noaa.gov/products/noaa-planetary-k-index.json", payload) &&
        !deserializeJson(doc, payload)) {
      JsonArray rows = doc.as<JsonArray>();
      int n = rows.size();
      if (n > 0) {
        // Last element is the most recent reading
        JsonVariant last = rows[n - 1];
        float kp = 0.0f;
        if (last.is<JsonObject>()) {
          // New format: array of objects with "Kp" key
          kp = last["Kp"].as<float>();
        } else {
          // Old format: array of arrays, Kp is index 1
          kp = String((const char*)last[1]).toFloat();
        }
        geomagneticState = geomagneticLabel(kp);
        Serial.printf("NOAA Kp: %.2f -> %s\n", kp, geomagneticState.c_str());
      }
    }
  }

  {
    String payload;
    DynamicJsonDocument doc(12000);
    DeserializationError error;
    if (fetchPayload("https://services.swpc.noaa.gov/products/solar-wind/plasma-2-hour.json", payload) &&
        !(error = deserializeJson(doc, payload))) {
      JsonArray rows = doc.as<JsonArray>();
      for (int i = rows.size() - 1; i > 0; i--) {
        if (!rows[i][2].isNull()) {
          float value = rows[i][2].as<float>();
          if (value > 0) {
            solarWindSpeed = String((int)(value + 0.5f)) + " km/s";
            windOk = true;
          }
          break;
        }
      }
      Serial.printf("NOAA solar wind: %s\n", solarWindSpeed.c_str());
    } else if (error) {
      Serial.printf("NOAA wind JSON error: %s\n", error.c_str());
    }
  }

  {
    String payload;
    DynamicJsonDocument doc(4000);
    DeserializationError error;
    if (fetchPayload("https://services.swpc.noaa.gov/json/goes/primary/xray-flares-latest.json", payload) &&
        !(error = deserializeJson(doc, payload))) {
      time_t now;
      time(&now);
      struct tm utcNow;
      if (gmtime_r(&now, &utcNow)) {
        char today[11];
        strftime(today, sizeof(today), "%Y-%m-%d", &utcNow);
        int count = 0;
        for (JsonObject flare : doc.as<JsonArray>()) {
          const char* beginTime = flare["begin_time"];
          if (beginTime && strncmp(beginTime, today, 10) == 0) count++;
        }
        flareCountToday = String(count) + " today";
        flaresOk = true;
        Serial.printf("NOAA flares: %s\n", flareCountToday.c_str());
      }
    } else if (error) {
      Serial.printf("NOAA flare JSON error: %s\n", error.c_str());
    }
  }

  return windOk && flaresOk;
}

bool maybeFetchSpaceWeather() {
  unsigned long now = millis();
  if (lastSpaceWeatherFetchMs != 0 &&
      now - lastSpaceWeatherFetchMs < SPACE_WEATHER_INTERVAL_SEC * 1000UL) {
    return false;
  }
  if (lastSpaceWeatherAttemptMs != 0 && now - lastSpaceWeatherAttemptMs < 15000UL) return false;

  // On first entry (lastSpaceWeatherFetchMs == 0) wait 3 seconds so the
  // orrery renders before blocking HTTPS calls start.
  if (lastSpaceWeatherFetchMs == 0 && lastSpaceWeatherAttemptMs == 0) {
    lastSpaceWeatherAttemptMs = now; // start the 3-second guard
    return false;
  }

  lastSpaceWeatherAttemptMs = now;
  if (fetchSpaceWeather()) {
    lastSpaceWeatherFetchMs = now;
    return true;
  }
  return false;
}

double moonAgeDays(time_t now) {
  const double synodicMonth = 29.53058867;
  double age = fmod(unixToJD(now) - 2451550.1, synodicMonth);
  if (age < 0) age += synodicMonth;
  return age;
}

String moonPhaseName(double age) {
  if (age < 1.85 || age >= 27.68) return "New Moon";
  if (age < 5.54) return "Waxing Crescent";
  if (age < 9.23) return "First Quarter";
  if (age < 12.92) return "Waxing Gibbous";
  if (age < 16.61) return "Full Moon";
  if (age < 20.30) return "Waning Gibbous";
  if (age < 23.99) return "Last Quarter";
  return "Waning Crescent";
}

void drawCometIcon(int x, int y, uint16_t color, uint16_t bg) {
  tft.drawLine(x, y + 5, x + 8, y, color);
  tft.drawLine(x + 1, y + 8, x + 9, y + 2, color);
  tft.fillCircle(x + 10, y + 1, 2, color);
}

void drawMoonIcon(int x, int y, uint16_t color, uint16_t bg) {
  tft.fillCircle(x + 5, y + 5, 5, color);
  tft.fillCircle(x + 8, y + 3, 5, bg);
}

String planetClockLabel(int idx, time_t now) {
  double daySeconds = (double)PLANET_DAY_HOURS[idx] * 3600.0;
  double localSeconds = fmod((double)now, daySeconds);
  if (localSeconds < 0) localSeconds += daySeconds;

  int hour = (int)(localSeconds * 24.0 / daySeconds);
  int minute = (int)(fmod(localSeconds, daySeconds / 24.0) * 60.0 / (daySeconds / 24.0));
  char buf[28];
  snprintf(buf, sizeof(buf), "%s  %02d:%02d", PLANET_NAMES[idx], hour, minute);
  return String(buf);
}

void drawSelectedPlanetBanner() {
  tft.fillRect(58, 9, 204, 22, C_BG);
  if (selectedPlanet < 0) return;

  time_t now;
  time(&now);
  String label = planetClockLabel(selectedPlanet, now);
  tft.drawRoundRect(58, 9, 204, 22, 5, C_PUR_DIM);
  tft.setTextFont(2);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(C_WHITE, C_BG);
  tft.drawString(label, SW / 2, 21);
  tft.setTextDatum(TL_DATUM);
}

void drawSolarInfoPanel() {
  const int x = 171, y = 126, w = 143, h = 108;
  const uint16_t panelBg = tft.color565(6, 0, 14);
  time_t now;
  time(&now);
  double age = moonAgeDays(now);
  double illumination = (1.0 - cos(2.0 * M_PI * age / 29.53058867)) * 50.0;

  tft.fillRoundRect(x, y, w, h, 5, panelBg);
  tft.drawRoundRect(x, y, w, h, 5, C_PUR_DIM);
  tft.setTextDatum(TL_DATUM);
  tft.setTextFont(1);

  drawMoonIcon(x + 8, y + 8, C_PUR, panelBg);
  tft.setTextColor(C_PUR, panelBg);
  tft.drawString("SPACE WEATHER", x + 25, y + 9);

  tft.setTextColor(C_WHITE, panelBg);
  tft.drawString(moonPhaseName(age), x + 8, y + 34);
  tft.drawString(String((int)(illumination + 0.5)) + "% Illuminated", x + 8, y + 51);
  tft.setTextColor(C_PUR_DIM, panelBg);
  tft.drawString("Age:", x + 8, y + 76);
  tft.setTextColor(C_WHITE, panelBg);
  tft.setTextDatum(MR_DATUM);
  tft.drawString(String(age, 1) + " days", x + w - 8, y + 76);
  tft.setTextDatum(TL_DATUM);
}

double unixToJD(time_t t) { return 2440587.5 + (double)t / 86400.0; }
double JD_to_T(double jd) { return (jd - 2451545.0) / 36525.0; }

void drawOrrery(double T) {
  drawSolarFrame();

  for (int i = PLANET_COUNT - 1; i >= 0; i--) drawOrbit(P[i].orb_a, P[i].orb_b, C_ORBIT);
  drawSun();

  int sx[PLANET_COUNT], sy[PLANET_COUNT], order[PLANET_COUNT];
  for (int i = 0; i < PLANET_COUNT; i++) {
    double ang = planetAngle(i, T);
    angleToScreen(P[i].orb_a, P[i].orb_b, ang, sx[i], sy[i]);
    lastPlanetX[i] = sx[i];
    lastPlanetY[i] = sy[i];
    order[i] = i;
  }

  for (int i = 0; i < PLANET_COUNT - 1; i++) {
    for (int j = i + 1; j < PLANET_COUNT; j++) {
      if (sy[order[i]] > sy[order[j]]) {
        int tmp = order[i];
        order[i] = order[j];
        order[j] = tmp;
      }
    }
  }

  for (int k = 0; k < PLANET_COUNT; k++) {
    int i = order[k];
    if (sx[i] < 0 || sx[i] >= SW || sy[i] < 0 || sy[i] >= SH) continue;

    int r = P[i].dot_r;
    tft.fillCircle(sx[i], sy[i], r + 2, tft.color565(40, 0, 70));
    tft.fillCircle(sx[i], sy[i], r, PCOL[i]);

    if (i == 5) {
      drawSaturnRings(sx[i], sy[i], r, tft.color565(100, 30, 160));
      tft.fillCircle(sx[i], sy[i], r, PCOL[i]);
    }

    if (i == 4) drawJupiterBands(sx[i], sy[i], r);
    tft.fillCircle(sx[i] - 1, sy[i] - 1, 1, tft.color565(200, 100, 255));
  }

  drawSolarInfoPanel();
  drawSelectedPlanetBanner();
}

bool updateSelectedPlanetFromTouch() {
  int tx, ty;
  bool down = readTouchPoint(tx, ty);

  if (!down) {
    solarTouchWasDown = false;
    return false;
  }

  if (solarTouchWasDown) return false;
  solarTouchWasDown = true;

  int best = -1;
  long bestD2 = 999999L;
  for (int i = 0; i < PLANET_COUNT; i++) {
    int hitR = max(P[i].dot_r + 8, 11);
    long dx = tx - lastPlanetX[i];
    long dy = ty - lastPlanetY[i];
    long d2 = dx * dx + dy * dy;
    if (d2 <= (long)hitR * hitR && d2 < bestD2) {
      best = i;
      bestD2 = d2;
    }
  }

  if (best < 0 || best == selectedPlanet) return false;
  selectedPlanet = best;
  return true;
}

void showSolarScreen() {
  currentScreen = SCREEN_SOLAR;
  solarTouchWasDown = false;
  time_t now;
  time(&now);
  drawOrrery(JD_to_T(unixToJD(now)));
}

void renderSolarSnapshot() {
  time_t now;
  time(&now);
  drawOrrery(JD_to_T(unixToJD(now)));
}

void updateSolarScreen() {
  static int lastSolarMinute = -1;

  struct tm t;
  bool redraw = false;
  if (getLocalTime(&t) && t.tm_min != lastSolarMinute) {
    lastSolarMinute = t.tm_min;
    redraw = true;
  }
  if (updateSelectedPlanetFromTouch()) redraw = true;
  if (redraw) {
    time_t now;
    time(&now);
    drawOrrery(JD_to_T(unixToJD(now)));
  }
}
