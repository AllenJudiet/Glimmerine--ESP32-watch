# Glimmerine V0 (ESP32-C3 Watch k)

Glimmerine V0 is an ESP32-C3 powered desk clock and interactive gadget featuring a 320x240 ILI9341 TFT display, XPT2046 touchscreen control, and hardware navigation buttons. It switches between multiple utility and entertainment screens (Clock, Weather, Solar System Orrery, Snake Game, Doodle Pad, and System Info) using scroll transitions.

---

## Features

- **Glimmerine V0 Clock**: Clean sliding-drum digit display with real-time NTP sync.
- **Weather Screen**: Smart smooth weather animations (pulsing sun/smooth falling rain) and real-time temperature/conditions pulled from OpenWeatherMap, with custom vector typography.
- **Solar Orrery**: Smooth real-time orbits for Mercury, Venus, Earth, and Mars around the Sun.
- **Snake Game**: Touch/Button controlled classic retro game.
- **Doodle Pad**: High-performance black canvas with smooth drawing using 2-pixel anti-aliased style strokes.
- **System Info**: Hardware diagnostics, CPU status, and RAM monitoring.

---

## 🔌 Pin Mapping (ESP32-C3 / ESP32)

| Component | Pin Function | ESP32 GPIO | Description |
|---|---|---|---|
| **ILI9341 Screen** | SCK | 18 | SPI Clock |
| | MISO | 19 | SPI Master In Slave Out |
| | MOSI | 23 | SPI Master Out Slave In |
| | TFT_CS | 5 | TFT Chip Select |
| | TFT_DC | 2 | TFT Data/Command Select |
| | TFT_RST | 4 | TFT Reset |
| **Touch Screen** | TOUCH_CS | 15 | Touch Chip Select |
| | TOUCH_IRQ | 27 | Touch Interrupt Request |
| **Navigation Buttons**| BTN_UP | 26 | Hardware Nav UP (Active LOW) |
| | BTN_SELECT | 25 | Hardware Nav SELECT (Active LOW) |
| | BTN_DOWN | 33 | Hardware Nav DOWN (Active LOW) |

---

##  Libraries Used

This project relies on the following Arduino libraries:
- **[Adafruit GFX Library](https://github.com/adafruit/Adafruit-GFX-Library)**: Core graphics rendering, shapes, and custom fonts.
- **[Adafruit ILI9341 Library](https://github.com/adafruit/Adafruit_ILI9341)**: Display driver for ILI9341.
- **[XPT2046 Touchscreen](https://github.com/PaulStoffregen/XPT2046_Touchscreen)**: SPI touch controller driver.
- **[ArduinoJson](https://github.com/bblanchon/ArduinoJson)**: Lightweight JSON parsing for weather API responses.
- **ESP32 Standard Libraries**:
  - `WiFi` & `WiFiClientSecure`: Network connections.
  - `HTTPClient`: OpenWeatherMap REST requests.
  - `esp_wifi`: Power saving/performance tuning.

---

##  APIs Reference

1. **Time Sync**:
   - Protocol: Network Time Protocol (NTP)
   - Server: `pool.ntp.org`
2. **Weather Service**:
   - Provider: [OpenWeatherMap API](https://openweathermap.org/api)
   - Endpoint: `http://api.openweathermap.org/data/2.5/weather`
