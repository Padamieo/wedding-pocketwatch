#include <M5Unified.h>
#include "basepage.h"

void updateContentPage(size_t cursor_index); 

#define EYE_RGB(r, g, b) ((uint16_t)((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3)))

class HelloPage : public BasePage {
  public:
    void init() override {
      if (initialized) {
        return;
      }
      M5.Display.fillScreen(TFT_DARKGREEN);
      M5.Display.setCursor(100, 100);
      M5.Display.print("HelloPage init");
      lastLevel = -2;
      last_ms = 0;
      initialized = true;
      update();
    }

    void update() override {
      // Battery level changes slowly, so poll once a second
      uint32_t ms = M5.millis();
      if (last_ms && ms - last_ms < 1000) {
        return;
      }
      last_ms = ms;
      int level = M5.Power.getBatteryLevel();      // 0..100, or <0 if unknown
      bool charging = M5.Power.isCharging() == m5::Power_Class::is_charging_t::is_charging;
      if (level == lastLevel && charging == lastCharging) {
        return;
      }
      M5.Log.printf("battery: level=%d charging=%d\n", level, charging);
      lastLevel = level;
      lastCharging = charging;
      drawBattery(level, charging);
    }

  private:
    int lastLevel = -2;
    bool lastCharging = false;
    uint32_t last_ms = 0;

    void drawBattery(int level, bool charging) {
      int w = M5.Display.width(), h = M5.Display.height();
      // Battery at the top centre, low enough to be visible on round
      // screens (e.g. M5Dial) where the corners are cut off.
      // Layout: [ 76% ][battery icon], centred as a group.
      int bw = 44, bh = 20, textW = 60;
      int groupW = textW + 6 + bw + 4;
      int gx = (w - groupW) / 2;
      int bx = gx + textW + 6, by = h / 8;
      M5.Display.fillRect(gx, by - 2, groupW, bh + 4, TFT_DARKGREEN);  // clear old
      M5.Display.drawRect(bx, by, bw, bh, TFT_WHITE);
      M5.Display.fillRect(bx + bw, by + bh / 4, 4, bh / 2, TFT_WHITE);       // terminal

      char buf[12];
      if (level >= 0) {
        uint16_t col = level > 50 ? TFT_GREEN : level > 20 ? TFT_YELLOW : TFT_RED;
        int fillW = (bw - 4) * constrain(level, 0, 100) / 100;
        M5.Display.fillRect(bx + 2, by + 2, fillW, bh - 4, col);
        snprintf(buf, sizeof(buf), "%s%d%%", charging ? "+" : "", level);
      } else {
        snprintf(buf, sizeof(buf), "--%%");  // board can't report battery level
      }
      M5.Display.setFont(&fonts::DejaVu18);
      M5.Display.setTextSize(1);
      M5.Display.setTextDatum(textdatum_t::middle_right);
      M5.Display.setTextColor(TFT_WHITE, TFT_DARKGREEN);
      M5.Display.drawString(buf, bx - 6, by + bh / 2);
    }
};

class ClockPage : public BasePage {
  public:
    bool analog = false;
    int lastSecond = -1;

    // Off-screen buffer for the analog face (falls back to direct drawing)
    M5Canvas canvas{&M5.Display};
    bool canvasReady = false;
    int faceR = 0;

    // Called once from setup(). Time comes from the battery-backed RTC,
    // so it survives power-off. If the RTC has never been set (or lost its
    // backup power) it is seeded once from the sketch's compile time.
    static void initTime() {
      if (!M5.Rtc.isEnabled()) {
        M5.Log.println("No RTC on this board: time will reset on power-off");
        return;
      }
      auto dt = M5.Rtc.getDateTime();
      if (dt.date.year < 2024) {
        static const char* months = "JanFebMarAprMayJunJulAugSepOctNovDec";
        char mon[4] = {0};
        int day, year, hh, mm, ss;
        sscanf(__DATE__, "%3s %d %d", mon, &day, &year);
        sscanf(__TIME__, "%d:%d:%d", &hh, &mm, &ss);
        int month = (strstr(months, mon) - months) / 3 + 1;
        M5.Rtc.setDateTime({ { (int16_t)year, (int8_t)month, (int8_t)day }, { (int8_t)hh, (int8_t)mm, (int8_t)ss } });
      }
    }

    void toggleMode() {
      analog = !analog;
      initialized = false;  // full redraw on next render()
    }

    void init() override {
      if (initialized) {
        return;
      }
      M5.Display.fillScreen(TFT_BLACK);
      faceR = min((int)M5.Display.width(), (int)M5.Display.height()) * 9 / 20;
      if (analog && !canvasReady) {
        canvas.setPsram(true);
        canvas.setColorDepth(16);
        canvasReady = canvas.createSprite(faceR * 2 + 1, faceR * 2 + 1) != nullptr;
        if (!canvasReady) {
          canvas.setPsram(false);
          canvas.setColorDepth(8);
          canvasReady = canvas.createSprite(faceR * 2 + 1, faceR * 2 + 1) != nullptr;
        }
      }
      lastSecond = -1;
      initialized = true;
      update();
    }

    void update() override {
      m5::rtc_datetime_t dt;
      if (M5.Rtc.isEnabled()) {
        dt = M5.Rtc.getDateTime();
      } else {
        uint32_t t = M5.millis() / 1000;  // no RTC: uptime only
        dt.time = { (int8_t)(t / 3600 % 24), (int8_t)(t / 60 % 60), (int8_t)(t % 60) };
      }
      if (dt.time.seconds == lastSecond) {
        return;
      }
      lastSecond = dt.time.seconds;
      if (analog) {
        drawAnalog(dt);
      } else {
        drawDigital(dt);
      }
    }

  private:
    void drawDirect(const m5::rtc_datetime_t& dt, int cx, int cy) {
      // Same as face() but on the display; shift via a temporary sprite-less path
      int R = faceR;
      M5.Display.fillCircle(cx, cy, R, TFT_WHITE);
      M5.Display.fillCircle(cx, cy, R - 4, EYE_RGB(20, 20, 40));
      float s = dt.time.seconds / 60.0f;
      float m = (dt.time.minutes + s) / 60.0f;
      float h = ((dt.time.hours % 12) + m) / 12.0f;
      float fr[3] = { h, m, s };
      int len[3] = { R * 5 / 10, R * 8 / 10, R * 9 / 10 };
      uint16_t col[3] = { TFT_WHITE, TFT_LIGHTGREY, TFT_RED };
      for (int i = 0; i < 3; i++) {
        float a = fr[i] * 2 * M_PI - M_PI / 2;
        M5.Display.drawWideLine(cx, cy, cx + (int)(cosf(a) * len[i]), cy + (int)(sinf(a) * len[i]), 4.0f - i * 1.5f, col[i]);
      }
      M5.Display.fillCircle(cx, cy, 5, TFT_RED);
    }

    void drawDigital(const m5::rtc_datetime_t& dt) {
      int w = M5.Display.width(), h = M5.Display.height();
      char buf[16];
      M5.Display.setTextDatum(textdatum_t::middle_center);
      M5.Display.setTextPadding(w);  // background fill erases old digits, no flicker

      M5.Display.setFont(&fonts::Font7);  // 7-segment style
      M5.Display.setTextSize(1);
      // Scale HH:MM to fill ~95% of the screen width
      float timeScale = (w * 0.95f) / M5.Display.textWidth("88:88");
      M5.Display.setTextSize(timeScale);
      int timeH = M5.Display.fontHeight();
      M5.Display.setTextColor(TFT_GREEN, TFT_BLACK);
      snprintf(buf, sizeof(buf), "%02d:%02d", dt.time.hours, dt.time.minutes);
      M5.Display.drawString(buf, w / 2, h / 2 - timeH / 3);

      M5.Display.setTextSize(timeScale * 0.45f);  // seconds, same 7-segment font
      int secY = h / 2 - timeH / 3 + timeH / 2 + M5.Display.fontHeight() / 2 + 8;
      M5.Display.setTextColor(TFT_DARKGREEN, TFT_BLACK);
      snprintf(buf, sizeof(buf), "%02d", dt.time.seconds);
      M5.Display.drawString(buf, w / 2, secY);

      if (M5.Rtc.isEnabled()) {
        M5.Display.setFont(&fonts::DejaVu24);
        M5.Display.setTextSize(1);
        M5.Display.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
        snprintf(buf, sizeof(buf), "%04d-%02d-%02d", dt.date.year, dt.date.month, dt.date.date);
        M5.Display.drawString(buf, w / 2, min(h - 16, secY + timeH / 2 + 10));
      }
      M5.Display.setTextPadding(0);
    }

    template <typename T>
    void hand(T& g, int c, float frac, int len, int thick, uint16_t col) {
      float a = frac * 2 * M_PI - M_PI / 2;
      int x = c + (int)(cosf(a) * len), y = c + (int)(sinf(a) * len);
      g.drawWideLine(c, c, x, y, thick, col);
    }

    template <typename T>
    void face(T& g, int c, const m5::rtc_datetime_t& dt) {
      int R = faceR;
      g.fillRect(c - R, c - R, R * 2 + 1, R * 2 + 1, TFT_BLACK);
      g.fillCircle(c, c, R, TFT_WHITE);
      g.fillCircle(c, c, R - 4, EYE_RGB(20, 20, 40));
      for (int i = 0; i < 60; i++) {
        float a = i * M_PI / 30;
        int r0 = (i % 5) ? R - 10 : R - 20;
        g.drawLine(c + (int)(cosf(a) * r0), c + (int)(sinf(a) * r0),
                   c + (int)(cosf(a) * (R - 6)), c + (int)(sinf(a) * (R - 6)),
                   (i % 5) ? TFT_DARKGREY : TFT_WHITE);
      }
      float s = dt.time.seconds / 60.0f;
      float m = (dt.time.minutes + s) / 60.0f;
      float h = ((dt.time.hours % 12) + m) / 12.0f;
      hand(g, c, h, R * 5 / 10, 4.0f, TFT_WHITE);
      hand(g, c, m, R * 8 / 10, 3.0f, TFT_LIGHTGREY);
      hand(g, c, s, R * 9 / 10, 1.0f, TFT_RED);
      g.fillCircle(c, c, 5, TFT_RED);
    }

    void drawAnalog(const m5::rtc_datetime_t& dt) {
      int cx = M5.Display.width() / 2, cy = M5.Display.height() / 2;
      if (canvasReady) {
        face(canvas, faceR, dt);
        canvas.pushSprite(cx - faceR, cy - faceR);
      } else {
        // Fallback: draw straight to the screen centred on (cx, cy)
        // (face() assumes a square target, so draw on a centred square)
        drawDirect(dt, cx, cy);
      }
    }
};

class HexPage : public BasePage {
  public:
    static constexpr int ITEM_COUNT = 26;
    int lineH = 40;
    int scrollY = 0;        // pixels scrolled from the top
    int lastDrawnY = -1;
    float velocity = 0;     // px per frame, for a little fling after release
    int dragLastY = -1;

    // Off-screen buffer for the whole screen (falls back to direct drawing)
    M5Canvas canvas{&M5.Display};
    bool canvasReady = false;

    void init() override {
      if (initialized) {
        return;
      }
      lineH = M5.Display.height() / 6;
      if (!canvasReady) {
        canvas.setPsram(true);
        canvas.setColorDepth(16);
        canvasReady = canvas.createSprite(M5.Display.width(), M5.Display.height()) != nullptr;
        if (!canvasReady) {
          canvas.setPsram(false);
          canvas.setColorDepth(8);
          canvasReady = canvas.createSprite(M5.Display.width(), M5.Display.height()) != nullptr;
        }
      }
      M5.Display.fillScreen(TFT_BLACK);
      dragLastY = -1;
      velocity = 0;
      lastDrawnY = -1;
      initialized = true;
      update();
    }

    // Called from loop() for each touch point while this page is showing
    void onTouch(const m5::touch_detail_t& d) {
      if (d.isPressed()) {
        if (dragLastY >= 0) {
          int dy = d.y - dragLastY;
          scrollBy(-dy);
          velocity = -dy;
        } else {
          velocity = 0;
        }
        dragLastY = d.y;
      } else {
        dragLastY = -1;
      }
    }

    void releaseTouch() {
      dragLastY = -1;
    }

    void update() override {
      if (dragLastY < 0 && fabsf(velocity) > 0.5f) {
        scrollBy((int)velocity);  // fling after release
        velocity *= 0.92f;
      }
      if (scrollY == lastDrawnY) {
        return;
      }
      if (canvasReady) {
        drawList(canvas);
        canvas.pushSprite(0, 0);
      } else {
        drawList(M5.Display);
      }
      lastDrawnY = scrollY;
    }

  private:
    int maxScroll() const {
      return max(0, ITEM_COUNT * lineH - (int)M5.Display.height());
    }

    void scrollBy(int dy) {
      scrollY = constrain(scrollY + dy, 0, maxScroll());
      if (scrollY == 0 || scrollY == maxScroll()) {
        velocity = 0;
      }
    }

    template <typename T>
    void drawList(T& g) {
      int w = M5.Display.width(), h = M5.Display.height();
      g.fillRect(0, 0, w, h, TFT_BLACK);
      g.setFont(&fonts::DejaVu24);
      g.setTextSize(1);
      g.setTextDatum(textdatum_t::middle_center);
      int first = scrollY / lineH;
      for (int i = first; i < ITEM_COUNT; i++) {
        int y = i * lineH - scrollY;
        if (y > h) {
          break;
        }
        g.fillRect(0, y, w, lineH - 1, (i & 1) ? EYE_RGB(20, 30, 50) : EYE_RGB(10, 15, 30));
        char buf[16];
        char c = 'a' + i;
        snprintf(buf, sizeof(buf), "%c = %x", c, (unsigned)c);  // a = 61, b = 62, ...
        g.setTextColor(TFT_WHITE);
        g.drawString(buf, w / 2, y + lineH / 2);
      }
      // Scrollbar
      int ms = maxScroll();
      if (ms > 0) {
        int barH = max(20, h * h / (ITEM_COUNT * lineH));
        int barY = (h - barH) * scrollY / ms;
        g.fillRect(w - 5, barY, 4, barH, TFT_DARKGREY);
      }
    }
};


class EyePage : public BasePage {
  public:
    enum Effect {
      NORMAL, CAT, BLOOD, ZOMBIE, HEART, HYPNO, DOLLAR, BLOODSHOT, ROBOT, DEAD,
      SCARED, CUTE, STAR, DEMON, GOOGLY, CRYING, RAINBOW, SNAKE, ALIEN, SLEEPY,
      EFFECT_COUNT
    };

    int cx, cy, eyeR, pupilR;
    int effect = NORMAL;
    uint32_t phase = 0;      // animation frame counter
    uint32_t anim_ms = 0;
    bool closed = false;
    bool dirty = true;       // frame needs to be redrawn and pushed
    uint32_t next_ms = 0;

    // Off-screen back buffer covering the eye's bounding box
    M5Canvas canvas{&M5.Display};
    bool canvasReady = false;

    void init() override {
      if (initialized) {
        return;
      }
      cx = M5.Display.width() / 2;
      cy = M5.Display.height() / 2;
      eyeR = min((int)M5.Display.width(), (int)M5.Display.height()) * 2 / 5;
      pupilR = eyeR / 3;
      closed = false;
      next_ms = M5.millis() + 850;
      allocCanvas();
      M5.Display.fillScreen(TFT_BLACK);
      dirty = true;
      present();
      initialized = true;
    }

    // Called on screen press: switch to a different random effect
    void randomEffect() {
      int n;
      do {
        n = rand() % EFFECT_COUNT;
      } while (n == effect);
      effect = n;
      phase = 0;
      closed = false;
      next_ms = M5.millis() + 850;
      dirty = true;
    }

    void update() override {
      uint32_t ms = M5.millis();
      if (ms >= next_ms) {
        // Open for 850 ms + closed for 150 ms = one blink per second
        closed = !closed;
        next_ms = ms + (closed ? 150 : 850);
        dirty = true;
      }
      if (isAnimated() && ms - anim_ms >= 40) {
        anim_ms = ms;
        phase++;
        dirty = true;
      }
      present();
    }

  private:
    bool isAnimated() const {
      return effect == BLOOD || effect == HYPNO || effect == CRYING || effect == RAINBOW
          || effect == DEMON || effect == ROBOT || effect == GOOGLY;
    }

    // Try 16-bit in PSRAM, then 8-bit in internal RAM.
    // If both fail, frames are drawn directly to the display instead.
    void allocCanvas() {
      if (canvasReady) {
        return;
      }
      int size = eyeR * 2 + 1;
      canvas.setPsram(true);
      canvas.setColorDepth(16);
      if (canvas.createSprite(size, size)) {
        canvasReady = true;
        return;
      }
      canvas.setPsram(false);
      canvas.setColorDepth(8);
      canvasReady = canvas.createSprite(size, size) != nullptr;
    }

    static uint16_t hue(int h) {  // h: 0..359 -> fully saturated RGB565
      h %= 360;
      int x = (h % 60) * 255 / 60;
      switch (h / 60) {
        case 0: return EYE_RGB(255, x, 0);
        case 1: return EYE_RGB(255 - x, 255, 0);
        case 2: return EYE_RGB(0, 255, x);
        case 3: return EYE_RGB(0, 255 - x, 255);
        case 4: return EYE_RGB(x, 0, 255);
        default: return EYE_RGB(255, 0, 255 - x);
      }
    }

    template <typename T>
    void heart(T& g, int x, int y, int s, uint16_t c) {
      g.fillCircle(x - s / 2, y - s / 3, s / 2 + 1, c);
      g.fillCircle(x + s / 2, y - s / 3, s / 2 + 1, c);
      g.fillTriangle(x - s, y - s / 5, x + s, y - s / 5, x, y + s, c);
    }

    template <typename T>
    void star(T& g, int x, int y, int R, uint16_t c) {
      int r = R * 2 / 5;
      int px[10], py[10];
      for (int i = 0; i < 10; i++) {
        float a = -M_PI / 2 + i * M_PI / 5;
        int rr = (i & 1) ? r : R;
        px[i] = x + (int)(cosf(a) * rr);
        py[i] = y + (int)(sinf(a) * rr);
      }
      for (int i = 0; i < 10; i++) {
        int j = (i + 1) % 10;
        g.fillTriangle(x, y, px[i], py[i], px[j], py[j], c);
      }
    }

    template <typename T>
    void drop(T& g, int x, int y, int s, uint16_t c) {
      g.fillCircle(x, y, s, c);
      g.fillTriangle(x - s, y, x + s, y, x, y - s * 2, c);
    }

    template <typename T>
    void veins(T& g, int ex, int ey, uint16_t c, int count) {
      for (int i = 0; i < count; i++) {
        float a = i * 2 * M_PI / count + (i % 3) * 0.2f;
        int x0 = ex + (int)(cosf(a) * eyeR * 0.95f);
        int y0 = ey + (int)(sinf(a) * eyeR * 0.95f);
        int x1 = ex + (int)(cosf(a + 0.15f) * eyeR * 0.55f);
        int y1 = ey + (int)(sinf(a + 0.15f) * eyeR * 0.55f);
        g.drawLine(x0, y0, x1, y1, c);
        g.drawLine(x0 + 1, y0, x1 + 1, y1, c);
      }
    }

    template <typename T>
    void basicPupil(T& g, int x, int y, int r, uint16_t iris) {
      g.fillCircle(x, y, r, iris);
      g.fillCircle(x, y, r / 2, TFT_BLACK);
      g.fillCircle(x - r / 3, y - r / 3, max(2, r / 6), TFT_WHITE);
    }

    // Draw the whole eye into a target whose (ex, ey) is the eye centre
    template <typename T>
    void drawEye(T& g, int ex, int ey) {
      g.fillRect(ex - eyeR, ey - eyeR, eyeR * 2 + 1, eyeR * 2 + 1, TFT_BLACK);
      if (closed) {
        g.fillRect(ex - eyeR, ey - 3, eyeR * 2, 6, TFT_WHITE);  // eyelid line
        return;
      }
      int R = eyeR, P = pupilR;
      switch (effect) {
        case NORMAL:
          g.fillCircle(ex, ey, R, TFT_WHITE);
          basicPupil(g, ex, ey, P, TFT_BLUE);
          break;

        case CAT:
          g.fillCircle(ex, ey, R, EYE_RGB(200, 220, 40));
          g.fillCircle(ex, ey, R * 3 / 4, EYE_RGB(230, 170, 20));
          g.fillEllipse(ex, ey, P / 4, R * 4 / 5, TFT_BLACK);
          g.fillCircle(ex - R / 3, ey - R / 3, R / 10, TFT_WHITE);
          break;

        case BLOOD: {
          g.fillCircle(ex, ey, R, TFT_WHITE);
          veins(g, ex, ey, EYE_RGB(200, 0, 0), 10);
          basicPupil(g, ex, ey, P, EYE_RGB(140, 0, 0));
          for (int i = 0; i < 5; i++) {
            int x = ex - R + R / 3 + i * R * 2 / 5;
            int y = ey - R + (int)((phase * (3 + i) + i * 53) % (R * 2));
            drop(g, x, y, R / 14 + 3, EYE_RGB(180, 0, 0));
          }
          break;
        }

        case ZOMBIE:
          g.fillCircle(ex, ey, R, EYE_RGB(170, 190, 130));
          veins(g, ex, ey, EYE_RGB(80, 110, 40), 8);
          g.fillCircle(ex + R / 8, ey + R / 10, P, EYE_RGB(215, 215, 200));
          g.fillCircle(ex + R / 8, ey + R / 10, P / 2, EYE_RGB(185, 185, 175));
          break;

        case HEART:
          g.fillCircle(ex, ey, R, TFT_WHITE);
          heart(g, ex, ey, P + P / 3, EYE_RGB(230, 0, 80));
          g.fillCircle(ex - P / 2, ey - P / 2, max(2, P / 6), TFT_WHITE);
          break;

        case HYPNO:
          g.fillCircle(ex, ey, R, TFT_WHITE);
          for (int t = 0; t < 160; t++) {
            float a = t * 0.18f - phase * 0.25f;
            int rr = t * (R - 6) / 160;
            g.fillCircle(ex + (int)(cosf(a) * rr), ey + (int)(sinf(a) * rr), 3, EYE_RGB(120, 0, 160));
          }
          break;

        case DOLLAR:
          g.fillCircle(ex, ey, R, TFT_WHITE);
          g.fillCircle(ex, ey, P + P / 3, EYE_RGB(0, 160, 60));
          g.setFont(&fonts::DejaVu72);
          g.setTextSize(max(1.0f, P * 2.2f / 72));
          g.setTextDatum(textdatum_t::middle_center);
          g.setTextColor(EYE_RGB(255, 230, 0));
          g.drawString("$", ex, ey);
          break;

        case BLOODSHOT:
          g.fillCircle(ex, ey, R, EYE_RGB(255, 225, 225));
          veins(g, ex, ey, EYE_RGB(220, 0, 0), 18);
          basicPupil(g, ex, ey, P, EYE_RGB(110, 70, 30));
          break;

        case ROBOT: {
          g.fillCircle(ex, ey, R, EYE_RGB(60, 60, 70));
          g.drawCircle(ex, ey, R - 4, EYE_RGB(150, 150, 160));
          int glow = P + (int)(sinf(phase * 0.3f) * P / 5);
          g.fillCircle(ex, ey, glow, EYE_RGB(120, 0, 0));
          g.fillCircle(ex, ey, glow * 2 / 3, EYE_RGB(220, 0, 0));
          g.fillCircle(ex, ey, glow / 3, EYE_RGB(255, 120, 120));
          int sy = ey - R + (int)(phase * 6 % (R * 2));
          g.drawFastHLine(ex - R + 10, sy, R * 2 - 20, EYE_RGB(255, 60, 60));
          break;
        }

        case DEAD:
          g.fillCircle(ex, ey, R, TFT_WHITE);
          for (int o = -4; o <= 4; o++) {
            g.drawLine(ex - P + o, ey - P, ex + P + o, ey + P, TFT_BLACK);
            g.drawLine(ex + P + o, ey - P, ex - P + o, ey + P, TFT_BLACK);
          }
          break;

        case SCARED:
          g.fillCircle(ex, ey, R, TFT_WHITE);
          g.fillCircle(ex, ey, P / 4, TFT_BLACK);
          break;

        case CUTE:
          g.fillCircle(ex, ey, R, TFT_WHITE);
          g.fillCircle(ex, ey, R * 3 / 4, EYE_RGB(90, 50, 20));
          g.fillCircle(ex, ey, R / 2, TFT_BLACK);
          g.fillCircle(ex - R / 4, ey - R / 4, R / 7, TFT_WHITE);
          g.fillCircle(ex + R / 5, ey + R / 5, R / 14, TFT_WHITE);
          break;

        case STAR:
          g.fillCircle(ex, ey, R, TFT_WHITE);
          star(g, ex, ey, P + P / 2, EYE_RGB(255, 200, 0));
          break;

        case DEMON: {
          g.fillCircle(ex, ey, R, TFT_BLACK);
          int flick = (int)(sinf(phase * 0.7f) * P / 6);
          g.fillCircle(ex, ey, R * 3 / 5 + flick, EYE_RGB(255, 80, 0));
          g.fillCircle(ex, ey, R * 2 / 5, EYE_RGB(220, 0, 0));
          g.fillEllipse(ex, ey, P / 5, R / 2, TFT_BLACK);
          g.drawCircle(ex, ey, R - 1, EYE_RGB(120, 0, 0));
          break;
        }

        case GOOGLY: {
          g.fillCircle(ex, ey, R, TFT_WHITE);
          g.drawCircle(ex, ey, R - 1, TFT_BLACK);
          int jx = (int)(sinf(phase * 0.5f) * P / 2);
          g.fillCircle(ex + jx, ey + R - P - 6, P, TFT_BLACK);
          break;
        }

        case CRYING:
          g.fillCircle(ex, ey, R, TFT_WHITE);
          basicPupil(g, ex, ey - R / 6, P, EYE_RGB(60, 120, 220));
          for (int i = 0; i < 3; i++) {
            int x = ex - R / 3 + i * R / 3;
            int y = ey + (int)((phase * 4 + i * 40) % R);
            drop(g, x, y, R / 12 + 3, EYE_RGB(80, 170, 255));
          }
          break;

        case RAINBOW:
          g.fillCircle(ex, ey, R, TFT_WHITE);
          for (int i = 0; i < 12; i++) {
            g.fillArc(ex, ey, P / 2, P + P / 3, i * 30, i * 30 + 30, hue(i * 30 + phase * 8));
          }
          g.fillCircle(ex, ey, P / 2, TFT_BLACK);
          break;

        case SNAKE:
          g.fillCircle(ex, ey, R, EYE_RGB(60, 160, 40));
          g.fillCircle(ex, ey, R * 2 / 3, EYE_RGB(150, 210, 30));
          g.fillEllipse(ex, ey, P / 8 + 1, R * 9 / 10, TFT_BLACK);
          break;

        case ALIEN:
          g.fillCircle(ex, ey, R, EYE_RGB(0, 200, 80));
          g.fillCircle(ex, ey, R - 6, TFT_BLACK);
          g.fillCircle(ex - R / 3, ey - R / 3, R / 8, EYE_RGB(200, 255, 200));
          g.fillCircle(ex + R / 4, ey + R / 4, R / 16, EYE_RGB(200, 255, 200));
          break;

        case SLEEPY:
          g.fillCircle(ex, ey, R, TFT_WHITE);
          basicPupil(g, ex, ey + P / 2, P, TFT_BLUE);
          g.fillArc(ex, ey, 0, R, 180, 360, EYE_RGB(230, 180, 150));   // heavy eyelid
          g.fillRect(ex - R, ey - 2, R * 2, 4, EYE_RGB(150, 100, 80));
          break;
      }
    }

    void present() {
      if (!dirty) {
        return;
      }
      if (canvasReady) {
        drawEye(canvas, eyeR, eyeR);
        canvas.pushSprite(cx - eyeR, cy - eyeR);  // single blit, no flicker
      } else {
        drawEye(M5.Display, cx, cy);
      }
      dirty = false;
    }
};

// Junior / Senior picker.
//  - Left half "Junior": a random animal (30), drawn full screen with code.
//  - Right half "Senior": a random famous person (100) or historic item (100).
// Tap the result to go back to the choice screen.
class GamePage : public BasePage {
  public:
    enum Mode { CHOOSE, RESULT };
    Mode mode = CHOOSE;

    static constexpr int ANIMAL_COUNT = 30;
    static constexpr int PERSON_COUNT = 100;
    static constexpr int ITEM_COUNT = 100;

    const char* animalNames[ANIMAL_COUNT] = {
      "Cat", "Dog", "Pig", "Cow", "Sheep", "Frog",
      "Fish", "Owl", "Penguin", "Mouse", "Rabbit", "Bear",
      "Panda", "Lion", "Tiger", "Fox", "Chick", "Duck",
      "Elephant", "Giraffe", "Monkey", "Koala", "Snail", "Ladybug",
      "Bee", "Butterfly", "Turtle", "Snake", "Octopus", "Whale"
    };
    const char* persons[PERSON_COUNT] = {
      "Albert Einstein", "Leonardo da Vinci", "Marie Curie", "Isaac Newton",
      "Cleopatra", "Julius Caesar", "Napoleon", "Mahatma Gandhi",
      "Nelson Mandela", "Martin Luther King", "Abraham Lincoln", "George Washington",
      "Winston Churchill", "Queen Victoria", "William Shakespeare", "Mozart",
      "Beethoven", "Pablo Picasso", "Vincent van Gogh", "Rembrandt",
      "Charles Darwin", "Galileo Galilei", "Nikola Tesla", "Thomas Edison",
      "Christopher Columbus", "Marco Polo", "Neil Armstrong", "Amelia Earhart",
      "Joan of Arc", "Genghis Khan", "Alexander the Great", "Confucius",
      "Aristotle", "Plato", "Socrates", "Frida Kahlo",
      "Charlie Chaplin", "Elvis Presley", "Michael Jackson", "Walt Disney",
      "Steve Jobs", "Florence Nightingale", "Anne Frank", "Queen Elizabeth II",
      "Henry VIII", "Tutankhamun", "Ada Lovelace", "Alan Turing",
      "Bill Gates", "Rosa Parks", "Mother Teresa", "Charles Dickens",
      "Jane Austen", "Mark Twain", "Johann Sebastian Bach", "Michelangelo",
      "Claude Monet", "Salvador Dali", "Andy Warhol", "Marilyn Monroe",
      "Audrey Hepburn", "John Lennon", "Freddie Mercury", "Bob Marley",
      "Muhammad Ali", "Pele", "Usain Bolt", "Serena Williams",
      "Johan Cruijff", "Yuri Gagarin", "Stephen Hawking", "Sigmund Freud",
      "Louis Pasteur", "Alexander Fleming", "Ferdinand Magellan", "James Cook",
      "Vasco da Gama", "Hannibal", "Charlemagne", "Augustus",
      "Nefertiti", "Ramses II", "Buddha", "Elizabeth I",
      "Catherine the Great", "Peter the Great", "John F. Kennedy", "Franklin D. Roosevelt",
      "Barack Obama", "Dalai Lama", "Harriet Tubman", "Benjamin Franklin",
      "Thomas Jefferson", "Coco Chanel", "Agatha Christie", "J.R.R. Tolkien",
      "Homer", "Nicolaus Copernicus", "Archimedes", "Pythagoras"
    };
    const char* items[ITEM_COUNT] = {
      "Mona Lisa", "Great Pyramid", "Eiffel Tower", "Great Wall of China",
      "Colosseum", "Stonehenge", "Statue of Liberty", "Taj Mahal",
      "Rosetta Stone", "Titanic", "Terracotta Army", "Big Ben",
      "Parthenon", "Machu Picchu", "Leaning Tower of Pisa", "Great Sphinx",
      "Berlin Wall", "Magna Carta", "Printing Press", "Telephone",
      "Light Bulb", "Steam Engine", "Model T Ford", "Apollo 11",
      "Hubble Telescope", "Sputnik", "Gutenberg Bible", "Dead Sea Scrolls",
      "Venus de Milo", "Statue of David", "The Night Watch", "The Scream",
      "Sistine Chapel", "Notre-Dame", "Tower of London", "Golden Gate Bridge",
      "Sydney Opera House", "Mount Rushmore", "Easter Island Heads", "Petra",
      "Angkor Wat", "Chichen Itza", "Trojan Horse", "Viking Longship",
      "Samurai Sword", "Crown Jewels", "Hope Diamond", "Enigma Machine",
      "Wright Flyer", "Compass", "Abacus", "Sundial",
      "Astrolabe", "Hourglass", "Microscope", "Gramophone",
      "Typewriter", "Penny Black", "ENIAC Computer", "The Wheel",
      "Windmill", "Spinning Jenny", "Steam Locomotive", "Hindenburg",
      "Concorde", "Space Shuttle", "Moon Rock", "Liberty Bell",
      "Declaration of Independence", "Bayeux Tapestry", "Book of Kells", "Girl with a Pearl Earring",
      "The Starry Night", "The Last Supper", "Birth of Venus", "Guernica",
      "Hanging Gardens", "Lighthouse of Alexandria", "Colossus of Rhodes", "Hagia Sophia",
      "Forbidden City", "Kremlin", "Brandenburg Gate", "Arc de Triomphe",
      "Neuschwanstein", "Alhambra", "Acropolis", "Pompeii",
      "Tutankhamun's Mask", "Mayflower", "HMS Victory", "Santa Maria",
      "Rubik's Cube", "Sony Walkman", "Polaroid Camera", "First iPhone",
      "Atari 2600", "Commodore 64", "Gameboy", "Morse Telegraph"
    };

    void init() override {
      if (initialized) {
        return;
      }
      mode = CHOOSE;
      drawChoice();
      initialized = true;
    }

    void update() override {}

    // Called from loop() on a new screen press while this page is showing
    void onTap(int x) {
      if (mode == CHOOSE) {
        mode = RESULT;
        if (x < (int)M5.Display.width() / 2) {
          drawAnimal(pickRandom(ANIMAL_COUNT, lastAnimal));
        } else {
          int n = pickRandom(PERSON_COUNT + ITEM_COUNT, lastSenior);
          if (n < PERSON_COUNT) {
            showText("Famous person", persons[n], EYE_RGB(110, 20, 90));
          } else {
            showText("Historic item", items[n - PERSON_COUNT], EYE_RGB(20, 50, 120));
          }
        }
      } else {
        mode = CHOOSE;
        drawChoice();
      }
    }

  private:
    int lastAnimal = -1, lastSenior = -1;
    int cx = 0, cy = 0, u = 1;  // drawing centre and unit size for animals

    static int pickRandom(int count, int& last) {
      int n;
      do {
        n = esp_random() % count;  // hardware RNG, different every boot
      } while (n == last && count > 1);
      last = n;
      return n;
    }

    void drawChoice() {
      int w = M5.Display.width(), h = M5.Display.height();
      M5.Display.fillRect(0, 0, w / 2, h, EYE_RGB(0, 150, 70));
      M5.Display.fillRect(w / 2, 0, w - w / 2, h, EYE_RGB(150, 30, 120));
      M5.Display.fillRect(w / 2 - 1, 0, 2, h, TFT_WHITE);
      M5.Display.setFont(&fonts::DejaVu24);
      M5.Display.setTextDatum(textdatum_t::middle_center);
      M5.Display.setTextSize(1);
      // Fit the longer word into half the screen
      float sz = min(2.0f, (w / 2 - 8) / (float)M5.Display.textWidth("Senior"));
      M5.Display.setTextSize(sz);
      M5.Display.setTextColor(TFT_WHITE);
      M5.Display.drawString("Junior", w / 4, h / 2);
      M5.Display.drawString("Senior", w * 3 / 4, h / 2);
    }

    // Big centred text with a small heading, wrapped onto two lines if needed
    void showText(const char* heading, const char* text, uint16_t bg) {
      int w = M5.Display.width(), h = M5.Display.height();
      M5.Display.fillScreen(bg);
      M5.Display.setTextDatum(textdatum_t::middle_center);
      M5.Display.setTextColor(TFT_WHITE);

      M5.Display.setFont(&fonts::DejaVu18);
      M5.Display.setTextSize(1);
      M5.Display.setTextColor(EYE_RGB(220, 220, 220));
      M5.Display.drawString(heading, w / 2, h / 6);
      M5.Display.setTextColor(TFT_WHITE);

      M5.Display.setFont(&fonts::DejaVu40);
      M5.Display.setTextSize(1);
      int full = M5.Display.textWidth(text);
      if (full <= w - 16 || strchr(text, ' ') == nullptr) {
        M5.Display.setTextSize(min(2.0f, (w - 16) / (float)full));
        M5.Display.drawString(text, w / 2, h / 2);
        return;
      }
      // Split at the space closest to the middle
      int len = strlen(text), best = -1;
      for (int k = 0; k < len; k++) {
        if (text[k] == ' ' && (best < 0 || abs(k - len / 2) < abs(best - len / 2))) {
          best = k;
        }
      }
      char l1[48], l2[48];
      snprintf(l1, sizeof(l1), "%.*s", best, text);
      snprintf(l2, sizeof(l2), "%s", text + best + 1);
      int widest = max((int)M5.Display.textWidth(l1), (int)M5.Display.textWidth(l2));
      M5.Display.setTextSize(min(2.0f, (w - 16) / (float)widest));
      int lh = M5.Display.fontHeight();
      M5.Display.drawString(l1, w / 2, h / 2 - lh / 2);
      M5.Display.drawString(l2, w / 2, h / 2 + lh / 2);
    }

    // ---- Animal drawing helpers (k values are in units of u) ----
    int X(float k) const { return cx + (int)(k * u); }
    int Y(float k) const { return cy + (int)(k * u); }
    int S(float k) const { return max(1, (int)(k * u)); }

    void circ(float x, float y, float r, uint16_t c) { M5.Display.fillCircle(X(x), Y(y), S(r), c); }
    void ell(float x, float y, float rx, float ry, uint16_t c) { M5.Display.fillEllipse(X(x), Y(y), S(rx), S(ry), c); }
    void tri(float x0, float y0, float x1, float y1, float x2, float y2, uint16_t c) {
      M5.Display.fillTriangle(X(x0), Y(y0), X(x1), Y(y1), X(x2), Y(y2), c);
    }
    void line(float x0, float y0, float x1, float y1, float r, uint16_t c) {
      M5.Display.drawWideLine(X(x0), Y(y0), X(x1), Y(y1), max(1.0f, r * u), c);
    }
    // Pair of eyes: white with black pupil and a highlight
    void eyes(float y, float dx, float r) {
      for (int s = -1; s <= 1; s += 2) {
        circ(s * dx, y, r, TFT_WHITE);
        circ(s * dx, y, r * 0.55f, TFT_BLACK);
        circ(s * dx - r * 0.2f, y - r * 0.2f, r * 0.18f, TFT_WHITE);
      }
    }
    // Simple black dot eyes with a highlight
    void dots(float y, float dx, float r) {
      for (int s = -1; s <= 1; s += 2) {
        circ(s * dx, y, r, TFT_BLACK);
        circ(s * dx - r * 0.3f, y - r * 0.3f, r * 0.3f, TFT_WHITE);
      }
    }
    void smile(float x, float y, float r, uint16_t c = TFT_BLACK) {
      M5.Display.fillArc(X(x), Y(y), S(r), max(0, S(r) - S(0.25f)), 20, 160, c);
    }
    void whiskers(float y, float x0, float x1) {
      for (int s = -1; s <= 1; s += 2) {
        line(s * x0, y, s * x1, y - 0.5f, 0.06f, TFT_BLACK);
        line(s * x0, y + 0.3f, s * x1, y + 0.8f, 0.06f, TFT_BLACK);
      }
    }

    void drawAnimal(int i) {
      int w = M5.Display.width(), h = M5.Display.height();
      u = max(2, min(w, h) / 12);
      cx = w / 2;
      cy = h / 2 - u;

      static const uint16_t bgs[ANIMAL_COUNT] = {
        EYE_RGB(120, 200, 255), EYE_RGB(150, 220, 120), EYE_RGB(180, 230, 140), EYE_RGB(110, 190, 90),
        EYE_RGB(130, 200, 255), EYE_RGB(170, 230, 255), EYE_RGB(30, 90, 180), EYE_RGB(20, 20, 70),
        EYE_RGB(200, 235, 255), EYE_RGB(255, 230, 180), EYE_RGB(150, 210, 110), EYE_RGB(120, 180, 90),
        EYE_RGB(140, 200, 100), EYE_RGB(240, 210, 120), EYE_RGB(60, 140, 70), EYE_RGB(40, 90, 60),
        EYE_RGB(150, 220, 255), EYE_RGB(80, 160, 230), EYE_RGB(240, 200, 140), EYE_RGB(140, 210, 255),
        EYE_RGB(60, 150, 60), EYE_RGB(130, 180, 120), EYE_RGB(120, 200, 100), EYE_RGB(90, 180, 80),
        EYE_RGB(150, 210, 255), EYE_RGB(255, 220, 240), EYE_RGB(90, 180, 220), EYE_RGB(230, 210, 150),
        EYE_RGB(20, 70, 140), EYE_RGB(40, 100, 180)
      };
      M5.Display.fillScreen(bgs[i]);

      const uint16_t BLK = TFT_BLACK, WHT = TFT_WHITE;
      const uint16_t PINK = EYE_RGB(255, 140, 170), ORNG = EYE_RGB(255, 150, 30);
      switch (i) {
        case 0: {  // Cat
          uint16_t c = EYE_RGB(255, 160, 60);
          tri(-4, -1, -3.5f, -5.5f, -0.8f, -3.5f, c);
          tri(4, -1, 3.5f, -5.5f, 0.8f, -3.5f, c);
          tri(-3.4f, -2, -3.2f, -4.5f, -1.6f, -3.3f, PINK);
          tri(3.4f, -2, 3.2f, -4.5f, 1.6f, -3.3f, PINK);
          circ(0, 0, 4, c);
          for (int s = -1; s <= 1; s += 2) {
            ell(s * 1.6f, -0.8f, 0.9f, 1.1f, EYE_RGB(120, 220, 60));
            ell(s * 1.6f, -0.8f, 0.25f, 0.95f, BLK);
          }
          tri(-0.5f, 0.6f, 0.5f, 0.6f, 0, 1.2f, PINK);
          whiskers(1.3f, 1.2f, 4.8f);
          smile(-0.4f, 1.2f, 0.45f);
          smile(0.4f, 1.2f, 0.45f);
          break;
        }
        case 1: {  // Dog
          uint16_t dark = EYE_RGB(120, 70, 30);
          ell(-3.6f, 0.5f, 1.3f, 2.8f, dark);
          ell(3.6f, 0.5f, 1.3f, 2.8f, dark);
          circ(0, 0, 3.8f, EYE_RGB(210, 160, 100));
          ell(0, 1.6f, 2.1f, 1.6f, EYE_RGB(245, 225, 190));
          dots(-0.8f, 1.5f, 0.6f);
          circ(0, 3.1f, 0.7f, PINK);
          ell(0, 0.9f, 0.8f, 0.55f, BLK);
          smile(-0.6f, 1.6f, 0.6f);
          smile(0.6f, 1.6f, 0.6f);
          break;
        }
        case 2: {  // Pig
          uint16_t dark = EYE_RGB(240, 120, 150);
          tri(-3.6f, -2, -3.2f, -5, -1, -3.5f, dark);
          tri(3.6f, -2, 3.2f, -5, 1, -3.5f, dark);
          circ(0, 0, 4, EYE_RGB(255, 180, 200));
          ell(0, 1, 1.9f, 1.3f, dark);
          ell(-0.6f, 1, 0.3f, 0.5f, EYE_RGB(150, 50, 80));
          ell(0.6f, 1, 0.3f, 0.5f, EYE_RGB(150, 50, 80));
          dots(-1.2f, 1.6f, 0.5f);
          break;
        }
        case 3: {  // Cow
          tri(-2.5f, -3, -3.5f, -5.5f, -1.2f, -3.6f, EYE_RGB(240, 230, 200));
          tri(2.5f, -3, 3.5f, -5.5f, 1.2f, -3.6f, EYE_RGB(240, 230, 200));
          ell(-4, -1.5f, 1.3f, 0.6f, BLK);
          ell(4, -1.5f, 1.3f, 0.6f, BLK);
          ell(0, 0, 3.4f, 4, WHT);
          circ(-1.8f, -2, 1.1f, BLK);
          circ(2, 0.2f, 0.9f, BLK);
          ell(0, 2.6f, 2.6f, 1.5f, PINK);
          circ(-0.9f, 2.6f, 0.35f, EYE_RGB(150, 60, 80));
          circ(0.9f, 2.6f, 0.35f, EYE_RGB(150, 60, 80));
          dots(-0.9f, 1.4f, 0.55f);
          break;
        }
        case 4: {  // Sheep
          for (int k = 0; k < 10; k++) {
            float a = k * 2 * M_PI / 10;
            circ(cosf(a) * 3.4f, sinf(a) * 3.4f, 1.6f, WHT);
          }
          circ(0, 0, 3.2f, WHT);
          ell(-2.6f, -0.2f, 1.1f, 0.5f, EYE_RGB(40, 40, 40));
          ell(2.6f, -0.2f, 1.1f, 0.5f, EYE_RGB(40, 40, 40));
          ell(0, 0.6f, 1.9f, 2.5f, EYE_RGB(40, 40, 40));
          eyes(0, 0.8f, 0.55f);
          smile(0, 1.8f, 0.5f, WHT);
          break;
        }
        case 5: {  // Frog
          uint16_t c = EYE_RGB(70, 190, 70);
          circ(-2.2f, -2, 1.6f, c);
          circ(2.2f, -2, 1.6f, c);
          ell(0, 0.8f, 4.6f, 3, c);
          eyes(-2, 2.2f, 1.1f);
          smile(0, 0.2f, 3, EYE_RGB(20, 90, 20));
          circ(-3, 1.5f, 0.6f, PINK);
          circ(3, 1.5f, 0.6f, PINK);
          break;
        }
        case 6: {  // Fish
          tri(2.6f, 0, 5.2f, -2.6f, 5.2f, 2.6f, ORNG);
          ell(-0.5f, 0, 3.6f, 2.4f, ORNG);
          tri(-0.5f, -2.2f, 1.5f, -3.6f, 1.5f, -1.8f, EYE_RGB(255, 110, 0));
          for (int k = 0; k < 3; k++) {
            M5.Display.fillArc(X(0.2f + k * 0.9f), Y(0), S(1.6f), S(1.4f), -60, 60, EYE_RGB(255, 200, 120));
          }
          circ(-2.4f, -0.5f, 0.75f, WHT);
          circ(-2.5f, -0.5f, 0.4f, BLK);
          smile(-3.3f, 0.2f, 0.5f);
          M5.Display.drawCircle(X(-4.5f), Y(-2.5f), S(0.4f), WHT);
          M5.Display.drawCircle(X(-4.9f), Y(-3.7f), S(0.6f), WHT);
          M5.Display.drawCircle(X(-4.2f), Y(-5), S(0.3f), WHT);
          break;
        }
        case 7: {  // Owl
          uint16_t c = EYE_RGB(140, 90, 50);
          tri(-3, -2.5f, -3.2f, -5.2f, -1.2f, -3.6f, c);
          tri(3, -2.5f, 3.2f, -5.2f, 1.2f, -3.6f, c);
          ell(0, 0.5f, 3.6f, 4.4f, c);
          ell(0, 2, 2.4f, 2.6f, EYE_RGB(220, 190, 140));
          for (int s = -1; s <= 1; s += 2) {
            circ(s * 1.5f, -1, 1.4f, WHT);
            circ(s * 1.5f, -1, 1.0f, EYE_RGB(255, 200, 0));
            circ(s * 1.5f, -1, 0.55f, BLK);
          }
          tri(-0.5f, -0.2f, 0.5f, -0.2f, 0, 0.9f, ORNG);
          break;
        }
        case 8: {  // Penguin
          ell(0, 0.5f, 3.3f, 4.6f, EYE_RGB(30, 30, 40));
          ell(0, 1.3f, 2.4f, 3.6f, WHT);
          ell(-3.3f, 1.2f, 0.6f, 2.2f, EYE_RGB(30, 30, 40));
          ell(3.3f, 1.2f, 0.6f, 2.2f, EYE_RGB(30, 30, 40));
          dots(-1.5f, 0.9f, 0.45f);
          tri(-0.6f, -0.7f, 0.6f, -0.7f, 0, 0.3f, ORNG);
          ell(-1.2f, 5, 1, 0.4f, ORNG);
          ell(1.2f, 5, 1, 0.4f, ORNG);
          break;
        }
        case 9: {  // Mouse
          uint16_t c = EYE_RGB(170, 170, 180);
          circ(-3, -2.8f, 1.9f, c);
          circ(3, -2.8f, 1.9f, c);
          circ(-3, -2.8f, 1.2f, PINK);
          circ(3, -2.8f, 1.2f, PINK);
          circ(0, 0.5f, 3.2f, c);
          dots(-0.2f, 1.2f, 0.5f);
          circ(0, 1.4f, 0.5f, PINK);
          whiskers(1.5f, 0.8f, 4.2f);
          break;
        }
        case 10: {  // Rabbit
          ell(-1.3f, -4, 0.9f, 2.8f, WHT);
          ell(1.3f, -4, 0.9f, 2.8f, WHT);
          ell(-1.3f, -4, 0.45f, 2.2f, PINK);
          ell(1.3f, -4, 0.45f, 2.2f, PINK);
          circ(0, 0.8f, 3, WHT);
          dots(0.2f, 1.2f, 0.45f);
          tri(-0.4f, 1.2f, 0.4f, 1.2f, 0, 1.7f, PINK);
          M5.Display.fillRect(X(-0.4f), Y(2), S(0.8f), S(0.7f), WHT);
          M5.Display.drawRect(X(-0.4f), Y(2), S(0.8f), S(0.7f), EYE_RGB(150, 150, 150));
          circ(-2, 1.8f, 0.5f, PINK);
          circ(2, 1.8f, 0.5f, PINK);
          break;
        }
        case 11: {  // Bear
          uint16_t c = EYE_RGB(140, 90, 50), tanC = EYE_RGB(215, 170, 120);
          circ(-3, -3, 1.3f, c);
          circ(3, -3, 1.3f, c);
          circ(-3, -3, 0.7f, tanC);
          circ(3, -3, 0.7f, tanC);
          circ(0, 0, 3.8f, c);
          ell(0, 1.3f, 1.8f, 1.3f, tanC);
          ell(0, 0.7f, 0.7f, 0.5f, BLK);
          dots(-1, 1.5f, 0.5f);
          smile(0, 1.4f, 0.7f);
          break;
        }
        case 12: {  // Panda
          circ(-3, -3, 1.3f, BLK);
          circ(3, -3, 1.3f, BLK);
          circ(0, 0, 3.8f, WHT);
          ell(-1.5f, -0.5f, 1, 1.3f, BLK);
          ell(1.5f, -0.5f, 1, 1.3f, BLK);
          circ(-1.4f, -0.7f, 0.4f, WHT);
          circ(1.4f, -0.7f, 0.4f, WHT);
          circ(-1.4f, -0.7f, 0.2f, BLK);
          circ(1.4f, -0.7f, 0.2f, BLK);
          ell(0, 1, 0.6f, 0.4f, BLK);
          smile(0, 1.5f, 0.7f);
          break;
        }
        case 13: {  // Lion
          uint16_t mane = EYE_RGB(200, 110, 30);
          for (int k = 0; k < 12; k++) {
            float a = k * 2 * M_PI / 12;
            circ(cosf(a) * 3.8f, sinf(a) * 3.8f, 1.5f, mane);
          }
          circ(0, 0, 4, mane);
          circ(-2.3f, -2.3f, 0.8f, EYE_RGB(240, 190, 90));
          circ(2.3f, -2.3f, 0.8f, EYE_RGB(240, 190, 90));
          circ(0, 0, 3.1f, EYE_RGB(240, 190, 90));
          ell(0, 1.4f, 1.6f, 1.1f, EYE_RGB(255, 230, 170));
          dots(-0.8f, 1.3f, 0.5f);
          tri(-0.6f, 0.6f, 0.6f, 0.6f, 0, 1.3f, EYE_RGB(110, 60, 30));
          smile(-0.4f, 1.4f, 0.45f);
          smile(0.4f, 1.4f, 0.45f);
          break;
        }
        case 14: {  // Tiger
          uint16_t c = EYE_RGB(255, 140, 20);
          circ(-3, -3, 1.2f, c);
          circ(3, -3, 1.2f, c);
          circ(-3, -3, 0.6f, WHT);
          circ(3, -3, 0.6f, WHT);
          circ(0, 0, 4, c);
          ell(-1.3f, 1.6f, 1.7f, 1.3f, WHT);
          ell(1.3f, 1.6f, 1.7f, 1.3f, WHT);
          tri(-0.5f, -4, 0.5f, -4, 0, -2.4f, BLK);
          tri(-1.6f, -3.7f, -0.9f, -3.8f, -1.2f, -2.6f, BLK);
          tri(1.6f, -3.7f, 0.9f, -3.8f, 1.2f, -2.6f, BLK);
          for (int s = -1; s <= 1; s += 2) {
            tri(s * 4, -1, s * 4, -0.2f, s * 2.8f, -0.6f, BLK);
            tri(s * 3.9f, 0.6f, s * 3.8f, 1.4f, s * 2.8f, 1, BLK);
          }
          eyes(-0.9f, 1.5f, 0.6f);
          tri(-0.5f, 0.8f, 0.5f, 0.8f, 0, 1.4f, PINK);
          whiskers(1.8f, 1.5f, 4.5f);
          break;
        }
        case 15: {  // Fox
          uint16_t c = EYE_RGB(240, 110, 20);
          tri(-4.4f, -1.5f, -3.6f, -5.5f, -1.2f, -2.8f, c);
          tri(4.4f, -1.5f, 3.6f, -5.5f, 1.2f, -2.8f, c);
          tri(-3.6f, -2, -3.3f, -4.5f, -2, -3, BLK);
          tri(3.6f, -2, 3.3f, -4.5f, 2, -3, BLK);
          tri(-4.6f, -2.2f, 4.6f, -2.2f, 0, 3.6f, c);
          tri(-4.6f, -2.2f, -0.4f, 0.5f, 0, 3.6f, WHT);
          tri(4.6f, -2.2f, 0.4f, 0.5f, 0, 3.6f, WHT);
          circ(0, 3.2f, 0.55f, BLK);
          dots(-0.9f, 1.6f, 0.5f);
          break;
        }
        case 16: {  // Chick
          uint16_t c = EYE_RGB(255, 220, 40);
          circ(-0.6f, -3.6f, 0.6f, ORNG);
          circ(0.3f, -3.9f, 0.6f, ORNG);
          circ(0, 0, 3.6f, c);
          ell(-3.4f, 1, 0.8f, 1.6f, EYE_RGB(240, 190, 20));
          ell(3.4f, 1, 0.8f, 1.6f, EYE_RGB(240, 190, 20));
          dots(-0.8f, 1.3f, 0.55f);
          tri(-0.7f, 0.2f, 0.7f, 0.2f, 0, 1.3f, ORNG);
          circ(-2.2f, 0.9f, 0.5f, PINK);
          circ(2.2f, 0.9f, 0.5f, PINK);
          tri(-1.2f, 3.5f, -0.6f, 4.5f, -1.8f, 4.5f, ORNG);
          tri(1.2f, 3.5f, 0.6f, 4.5f, 1.8f, 4.5f, ORNG);
          break;
        }
        case 17: {  // Duck
          circ(0, -0.5f, 3.6f, EYE_RGB(255, 225, 50));
          dots(-1.6f, 1.3f, 0.55f);
          ell(0, 1.1f, 2.3f, 0.9f, ORNG);
          M5.Display.drawFastHLine(X(-2), Y(1.1f), S(4), EYE_RGB(200, 100, 0));
          circ(-2.4f, 0, 0.5f, PINK);
          circ(2.4f, 0, 0.5f, PINK);
          M5.Display.fillRect(X(-6), Y(3.5f), S(12), S(3), EYE_RGB(60, 130, 210));
          break;
        }
        case 18: {  // Elephant
          uint16_t c = EYE_RGB(150, 150, 165);
          ell(-3.6f, 0, 2.3f, 3, c);
          ell(3.6f, 0, 2.3f, 3, c);
          ell(-3.6f, 0, 1.5f, 2.2f, PINK);
          ell(3.6f, 0, 1.5f, 2.2f, PINK);
          circ(0, -0.5f, 3, c);
          line(0, 0.5f, 0, 3.8f, 0.8f, c);
          line(0, 3.8f, 1.2f, 4.4f, 0.7f, c);
          dots(-1.1f, 1.2f, 0.45f);
          tri(-1.4f, 1.4f, -0.9f, 1.4f, -2.2f, 2.6f, WHT);
          tri(1.4f, 1.4f, 0.9f, 1.4f, 2.2f, 2.6f, WHT);
          break;
        }
        case 19: {  // Giraffe
          uint16_t c = EYE_RGB(250, 200, 70), sp = EYE_RGB(180, 110, 40);
          M5.Display.fillRect(X(-1), Y(1), S(2), S(6), c);
          circ(-0.4f, 3, 0.5f, sp);
          circ(0.5f, 4.5f, 0.5f, sp);
          line(-0.8f, -2.5f, -1.2f, -4.6f, 0.15f, sp);
          line(0.8f, -2.5f, 1.2f, -4.6f, 0.15f, sp);
          circ(-1.2f, -4.6f, 0.45f, sp);
          circ(1.2f, -4.6f, 0.45f, sp);
          ell(-2.4f, -2.2f, 1, 0.45f, c);
          ell(2.4f, -2.2f, 1, 0.45f, c);
          ell(0, -1, 2.2f, 2.8f, c);
          circ(-1.2f, -2.6f, 0.4f, sp);
          circ(1.4f, -2, 0.35f, sp);
          ell(0, 0.8f, 1.6f, 1.1f, EYE_RGB(255, 225, 150));
          circ(-0.5f, 0.6f, 0.2f, sp);
          circ(0.5f, 0.6f, 0.2f, sp);
          dots(-1.2f, 1, 0.45f);
          break;
        }
        case 20: {  // Monkey
          uint16_t c = EYE_RGB(130, 80, 40), tanC = EYE_RGB(235, 195, 150);
          circ(-3.6f, 0, 1.2f, c);
          circ(3.6f, 0, 1.2f, c);
          circ(-3.6f, 0, 0.7f, tanC);
          circ(3.6f, 0, 0.7f, tanC);
          circ(0, 0, 3.5f, c);
          circ(-1, -0.6f, 1.4f, tanC);
          circ(1, -0.6f, 1.4f, tanC);
          ell(0, 1.2f, 2.3f, 1.6f, tanC);
          dots(-0.6f, 1, 0.5f);
          circ(-0.3f, 0.7f, 0.15f, c);
          circ(0.3f, 0.7f, 0.15f, c);
          smile(0, 1.2f, 1);
          break;
        }
        case 21: {  // Koala
          uint16_t c = EYE_RGB(160, 160, 170);
          circ(-3.3f, -2.4f, 1.9f, c);
          circ(3.3f, -2.4f, 1.9f, c);
          circ(-3.3f, -2.4f, 1.1f, WHT);
          circ(3.3f, -2.4f, 1.1f, WHT);
          circ(0, 0, 3.5f, c);
          dots(-0.8f, 1.6f, 0.45f);
          ell(0, 0.7f, 0.9f, 1.3f, EYE_RGB(40, 40, 50));
          break;
        }
        case 22: {  // Snail
          uint16_t body = EYE_RGB(230, 200, 150);
          ell(0, 2.5f, 4.8f, 1, body);
          circ(-3.8f, 1.2f, 1.2f, body);
          line(-4.1f, 0.4f, -4.6f, -1.6f, 0.12f, body);
          line(-3.5f, 0.4f, -3.4f, -1.6f, 0.12f, body);
          circ(-4.6f, -1.6f, 0.4f, BLK);
          circ(-3.4f, -1.6f, 0.4f, BLK);
          smile(-3.9f, 1.3f, 0.5f);
          circ(0.6f, 0, 2.8f, EYE_RGB(170, 90, 40));
          for (int k = 1; k <= 3; k++) {
            M5.Display.drawCircle(X(0.6f + k * 0.2f), Y(k * 0.15f), S(2.8f - k * 0.7f), EYE_RGB(240, 180, 110));
          }
          break;
        }
        case 23: {  // Ladybug
          circ(0, -3, 1.8f, BLK);
          circ(-0.7f, -3.4f, 0.35f, WHT);
          circ(0.7f, -3.4f, 0.35f, WHT);
          line(-0.6f, -4.4f, -1.4f, -5.5f, 0.1f, BLK);
          line(0.6f, -4.4f, 1.4f, -5.5f, 0.1f, BLK);
          circ(0, 0.6f, 3.8f, EYE_RGB(230, 20, 20));
          line(0, -3.1f, 0, 4.3f, 0.1f, BLK);
          circ(-1.8f, -0.8f, 0.7f, BLK);
          circ(1.8f, -0.8f, 0.7f, BLK);
          circ(-2, 1.8f, 0.6f, BLK);
          circ(2, 1.8f, 0.6f, BLK);
          circ(-0.9f, 3.2f, 0.5f, BLK);
          circ(0.9f, 3.2f, 0.5f, BLK);
          break;
        }
        case 24: {  // Bee
          ell(-0.8f, -2.4f, 1.3f, 1.8f, EYE_RGB(220, 240, 255));
          ell(1, -2.4f, 1.3f, 1.8f, EYE_RGB(220, 240, 255));
          tri(3.6f, 0.2f, 3.6f, 1.2f, 4.8f, 0.7f, BLK);
          ell(0.2f, 0.7f, 3.6f, 2.5f, EYE_RGB(255, 210, 0));
          ell(-0.6f, 0.7f, 0.4f, 2.4f, BLK);
          ell(1.1f, 0.7f, 0.4f, 2.3f, BLK);
          ell(2.6f, 0.7f, 0.35f, 1.6f, BLK);
          circ(-3.6f, 0.4f, 1.5f, BLK);
          circ(-4, 0, 0.45f, WHT);
          circ(-4, 0, 0.2f, BLK);
          line(-3.6f, -1, -4.4f, -2.8f, 0.1f, BLK);
          line(-3.2f, -1, -2.8f, -2.8f, 0.1f, BLK);
          break;
        }
        case 25: {  // Butterfly
          ell(-2.2f, -1.6f, 2.2f, 2.4f, EYE_RGB(150, 60, 220));
          ell(2.2f, -1.6f, 2.2f, 2.4f, EYE_RGB(150, 60, 220));
          ell(-1.8f, 2, 1.6f, 1.8f, EYE_RGB(255, 120, 200));
          ell(1.8f, 2, 1.6f, 1.8f, EYE_RGB(255, 120, 200));
          circ(-2.4f, -1.8f, 0.9f, EYE_RGB(255, 220, 60));
          circ(2.4f, -1.8f, 0.9f, EYE_RGB(255, 220, 60));
          circ(-1.8f, 2.2f, 0.6f, WHT);
          circ(1.8f, 2.2f, 0.6f, WHT);
          ell(0, 0.3f, 0.45f, 3.4f, BLK);
          line(0, -3, -1.2f, -5, 0.1f, BLK);
          line(0, -3, 1.2f, -5, 0.1f, BLK);
          circ(-1.2f, -5, 0.3f, BLK);
          circ(1.2f, -5, 0.3f, BLK);
          break;
        }
        case 26: {  // Turtle
          uint16_t skin = EYE_RGB(130, 200, 90);
          ell(-2.5f, 2.2f, 0.9f, 0.7f, skin);
          ell(2.5f, 2.2f, 0.9f, 0.7f, skin);
          tri(-3.6f, 0.8f, -4.8f, 1.6f, -3.4f, 1.6f, skin);
          circ(3.9f, -0.4f, 1.2f, skin);
          circ(4.3f, -0.8f, 0.3f, BLK);
          smile(4.1f, -0.3f, 0.5f);
          ell(0, 0.5f, 3.6f, 2.4f, EYE_RGB(40, 120, 50));
          circ(0, 0.2f, 0.9f, EYE_RGB(90, 170, 70));
          circ(-1.8f, 0.6f, 0.7f, EYE_RGB(90, 170, 70));
          circ(1.8f, 0.6f, 0.7f, EYE_RGB(90, 170, 70));
          circ(-0.9f, -1.1f, 0.6f, EYE_RGB(90, 170, 70));
          circ(0.9f, -1.1f, 0.6f, EYE_RGB(90, 170, 70));
          break;
        }
        case 27: {  // Snake
          uint16_t c = EYE_RGB(60, 170, 60);
          line(-4, 4, -1.5f, 3, 0.8f, c);
          line(-1.5f, 3, 1.5f, 2.6f, 0.8f, c);
          line(1.5f, 2.6f, 2.5f, 1, 0.8f, c);
          line(2.5f, 1, 0.5f, -0.3f, 0.8f, c);
          line(0.5f, -0.3f, -1.5f, -1, 0.8f, c);
          line(-1.5f, -1, -0.8f, -2.6f, 0.8f, c);
          ell(0.5f, -3, 1.8f, 1.3f, c);
          line(2.2f, -2.8f, 3.4f, -2.8f, 0.08f, EYE_RGB(230, 0, 0));
          tri(3.3f, -2.8f, 3.8f, -3.2f, 3.8f, -2.4f, EYE_RGB(230, 0, 0));
          eyes(-3.4f, 0.7f, 0.45f);
          break;
        }
        case 28: {  // Octopus
          uint16_t c = EYE_RGB(190, 80, 200);
          for (int k = 0; k < 6; k++) {
            float x0 = -2.5f + k * 1.0f;
            float x1 = -4 + k * 1.6f;
            line(x0, 1, x1, 3.6f, 0.45f, c);
            circ(x1 + ((k < 3) ? -0.3f : 0.3f), 4, 0.45f, c);
          }
          ell(0, -1, 3.2f, 3, c);
          eyes(-1.2f, 1.3f, 0.8f);
          smile(0, 0.2f, 0.9f);
          circ(-2.3f, 0.3f, 0.45f, PINK);
          circ(2.3f, 0.3f, 0.45f, PINK);
          break;
        }
        case 29: {  // Whale
          uint16_t c = EYE_RGB(60, 120, 220);
          line(0, -2, -0.6f, -4.5f, 0.15f, EYE_RGB(180, 230, 255));
          line(0, -2, 0.6f, -4.5f, 0.15f, EYE_RGB(180, 230, 255));
          line(0, -2, 0, -4.8f, 0.15f, EYE_RGB(180, 230, 255));
          tri(3, 0.8f, 5.2f, -1.4f, 5.2f, 2.6f, c);
          ell(-0.5f, 0.8f, 4, 2.6f, c);
          ell(-0.8f, 2, 3.2f, 1.2f, EYE_RGB(190, 220, 255));
          circ(-2.6f, 0.2f, 0.45f, BLK);
          smile(-3, 0.9f, 0.7f);
          break;
        }
      }

      // Name label at the bottom
      M5.Display.setFont(&fonts::DejaVu40);
      M5.Display.setTextSize(1);
      float sz = min(1.0f, (w - 16) / (float)M5.Display.textWidth(animalNames[i]));
      M5.Display.setTextSize(sz);
      M5.Display.setTextDatum(textdatum_t::bottom_center);
      M5.Display.setTextColor(TFT_WHITE);
      int lx = w / 2, ly = h - 8;
      // Dark outline so the label reads on any background
      M5.Display.setTextColor(TFT_BLACK);
      M5.Display.drawString(animalNames[i], lx - 2, ly);
      M5.Display.drawString(animalNames[i], lx + 2, ly);
      M5.Display.drawString(animalNames[i], lx, ly - 2);
      M5.Display.drawString(animalNames[i], lx, ly + 2);
      M5.Display.setTextColor(TFT_WHITE);
      M5.Display.drawString(animalNames[i], lx, ly);
    }
};

class Menu {
  public:
    struct menu_item_t {
      const char* title;
      void (*func)(size_t cursor_index);
    };
    bool visible = false;
    bool initialized = false;
    bool sound = false;
    int menu_x = 2;
    int menu_y = 20;
    int menu_w = 120;
    int menu_h = 30;
    int menu_padding = 25;
    size_t cursor_index = 0;
    menu_item_t menus[7] = {
      { "Hello", updateContentPage },
      { "Clock", updateContentPage },
      { "Hex", updateContentPage },
      { "Photos", updateContentPage },
      { "Schedule", updateContentPage },
      { "Eye", updateContentPage },
      { "Game", updateContentPage }
    };
    //size_t menu_count = 0;
    size_t menu_count = 7;

    // int myNum; // Attribute
    void init() { // Method
      if (initialized) {
        return;
      }
      // M5.Display.startWrite();

      // M5.Display.fillScreen(TFT_NAVY);
      
      int w = M5.Display.width() >> 3;
      int h = M5.Display.height() >> 3;
      for (int i = 0; i < 128; ++i) {
        uint16_t c = rand();
        M5.Display.fillRect(
          rand() % M5.Display.width(),
          rand() % M5.Display.height(),
          w, h, c
          //(i&1) ? TFT_BLACK:TFT_WHITE
        );
        M5.delay(1);
      }

      menu_w = M5.Display.width() >> 1;

        M5.Display.setFont(&fonts::DejaVu18);
        // if (M5.Display.width() > 400) {
        M5.Display.setTextSize(2);
          // menu_y *= 2;
          // menu_w = M5.Display.width() >> 1;
        //}

      static int x = M5.Lcd.width() / 2;

      menu_x = x - menu_w / 2;

      menu_padding = (M5.Display.height() - menu_y) / menu_count;
      menu_h = menu_padding - 2;
      
      M5.Display.setCursor(0, M5.Display.width() / 2);
      M5.Display.print("MENU");

      M5.Display.setCursor(menu_x, 0);

      M5.delay(10);

      for (size_t i = 0; i < menu_count; i++) {
        draw_menu(i, i == cursor_index);
      }
      // menus[2] 
      // constexpr const size_t menu_count = sizeof(menus) / sizeof(this.menus[0]);
      menu_count = sizeof(menus) / sizeof(menus[0]);

      initialized = true;
    }

    void draw_menu(size_t index, bool focus) {
      auto baseColor = M5.Display.getBaseColor();
      M5.Display.setColor(focus ? baseColor : ~baseColor);
      M5.Display.drawRect(menu_x  , menu_y + index * menu_padding  , menu_w  , menu_h  );
      M5.Display.drawRect(menu_x+1, menu_y + index * menu_padding+1, menu_w-2, menu_h-2);
      M5.Display.setColor(focus ? ~baseColor : baseColor);
      M5.Display.fillRect(menu_x+2, menu_y + index * menu_padding+2, menu_w-4, menu_h-4);
      M5.Display.setTextDatum(textdatum_t::middle_center);
      M5.Display.setTextColor(focus ? baseColor : ~baseColor, focus ? ~baseColor : baseColor);
      M5.Display.drawString(menus[index].title, menu_x + (menu_w >> 1), menu_y + index * menu_padding + (menu_h >> 1));
    }

    void select_menu(size_t index) {
      /// 操作音を鳴らす。
      float Hz = 880 * powf(2.0, index / 12.0f);
      sound && M5.Speaker.tone(Hz, 100);
      cursor_index = index;
    }

    void move_menu(bool back = false) {
      if (back) {
        select_menu((cursor_index ? cursor_index : menu_count) - 1);
      } else {
        select_menu((cursor_index + 1) % menu_count);
      }
    }

    void exec_menu(bool holding) {
      /// holding は長押し中とそれ以外で処理を変えたい場合に利用できる。
      if (holding == false) {
        sound && M5.Speaker.tone(880, 150);
      }

      if (menus[cursor_index].func != nullptr) {
        // M5.Display.setClipRect(menu_x + menu_w, 0, M5.Display.width(), M5.Display.height());
        // M5.Display.setClipRect(0, 0, M5.Display.width() - menu_w + 10, M5.Display.height());
        menus[cursor_index].func(cursor_index);
        visible = false;
        initialized = false;
        // M5.Display.clearClipRect();
        // M5.Display.fillScreen(TFT_AZURE);
      }
    }

    void update() {
      if(visible) {
        static size_t prev_index = 0;
        if (prev_index != cursor_index) {
          draw_menu(prev_index, false);
          draw_menu(cursor_index, true);
          prev_index = cursor_index;
        }
      }
    }
};


// BasePage contentPage;
BasePage* contentPage;

HexPage hexPage;
ClockPage clockPage;
HelloPage helloPage;
EyePage eyePage;
GamePage gamePage;
BasePage basePage;

Menu myObj;

void updateContentPage(size_t cursor_index) {
  if(cursor_index == 0) {
    contentPage = &helloPage;
  }
  if(cursor_index == 1) {
    contentPage = &clockPage;
  }
  if(cursor_index == 2) {
    contentPage = &hexPage;
  }
  if(cursor_index == 3) {
    contentPage = &basePage;
  }
  if(cursor_index == 4) {
    contentPage = &basePage;
  }
  if(cursor_index == 5) {
    contentPage = &eyePage;
  }
  if(cursor_index == 6) {
    contentPage = &gamePage;
  }
  contentPage->initialized = false;
}

void setup(void) {
  
  new(&myObj) Menu(); // reconstruct

  // BasePage contentPage = BasePage();
  // new(&contentPage) HexPage(); // reconstruct
  // BasePage *contentPage = new HexPage();
    // Creating a pointer of type Base  
  // Creating an object of Derived class
  

  // Pointing base class pointer to 
  // derived class object
  contentPage = &clockPage;
  // Calling the display function 
  // using base class pointer
  // contentPage->display();

  auto cfg = M5.config();
  M5.begin(cfg);
  Serial.begin(115200);

  if (M5.Display.width() > M5.Display.height()) {
    M5.Display.setRotation(M5.Display.getRotation() ^ 1);
  }
  // M5.Display.setRotation(M5.Display.getRotation() ^ 3);

  /// このサンプルでは、startWriteをしたまま、対になるendWriteを使わないようにする。
  // M5.Display.startWrite();

  M5.Display.setEpdMode(epd_mode_t::epd_fastest);

  ClockPage::initTime();

  contentPage->init();
}

void loop(void) {
  /// ディスプレイがビジー状態でない場合のみ処理する。
  /// EPDでは画面の更新中はBusyとなるためここを通らない。
  if (!M5.Display.displayBusy()) {
    // 選択しているメニューが変更されていれば再描画。
    if(myObj.visible) {
      myObj.update();
    } else {
      contentPage->render();
    }

    /// 表示内容を画面に反映する。
    M5.Display.display();

    /// ※ M5Paper, CoreInk, Unit OLED についてはここで画面が更新される。
  }
  { /// 10ミリ秒間隔で処理が進むように待機する。
    static uint32_t prev_ms;
    uint32_t ms = M5.millis();
    int diff = (10 - (ms - prev_ms));
    if (diff > 0) {
      ms += diff;
      M5.delay(diff);
    }
    prev_ms = ms;
  }
  M5.update();

  auto touch_count = M5.Touch.getCount();
  if (touch_count == 0) {
    hexPage.releaseTouch();  // finger lifted: end the drag so the fling starts
  }
  for (size_t i = 0; i < touch_count; i++) {
    auto detail = M5.Touch.getDetail(i);
    // Tap the eye page to switch to a random eye effect
    if (!myObj.visible && contentPage == &eyePage && detail.wasPressed()) {
      eyePage.randomEffect();
    }
    // Junior / Senior choice and "back" tap on the game page
    if (!myObj.visible && contentPage == &gamePage && detail.wasPressed()) {
      gamePage.onTap(detail.x);
    }
    // Drag on the hex page to scroll the list
    if (!myObj.visible && contentPage == &hexPage) {
      hexPage.onTouch(detail);
    }
    // Tap the clock page to switch between digital and analog
    if (!myObj.visible && contentPage == &clockPage && detail.wasPressed()) {
      clockPage.toggleMode();
    }
    if(myObj.visible) {
      if ( ((size_t)detail.x - myObj.menu_x) < myObj.menu_w) {
        size_t index = (detail.y - myObj.menu_y) / myObj.menu_padding;
        if (index < myObj.menu_count) {
          if (detail.wasPressed()) {
            myObj.select_menu(index);
            M5.Log.println("update");
          } else if (index == myObj.cursor_index) {
            if (detail.wasClicked()) {
              myObj.exec_menu(false);
            } else if (detail.isHolding()) {
              myObj.exec_menu(true);
            }
          }
        }
      }
    }
    //
  }

  switch (M5.getBoard()) {
  default:
    if (M5.BtnA.wasClicked()) {
      if(myObj.visible) {
        myObj.move_menu(false);
      } else {
        // myObj.visible = true;
        // myObj.init();
      }
    }
    // if (M5.BtnA.wasHold()) {
    //   myObj.exec_menu(false);
    // }
    if (M5.BtnA.isHolding() && M5.BtnB.isHolding()) {
      M5.Log.println("holding");
      if(!myObj.visible) {
        M5.Log.println("holding - visible");
        myObj.visible = true;
        myObj.init();
      }
    }
    if (M5.BtnB.wasClicked()) {
      if(myObj.visible) {
        myObj.move_menu(true);
      }
    }
    break;
  }
}

#if !defined ( ARDUINO )
extern "C" {
  void loopTask(void*) {
    setup();
    for (;;) {
      loop();
    }
    vTaskDelete(NULL);
  }

  void app_main() {
    xTaskCreatePinnedToCore(loopTask, "loopTask", 8192, NULL, 1, NULL, 1);
  }
}
#endif