#ifndef SCHEDULE_PAGE_H
#define SCHEDULE_PAGE_H

// Define the hex page
// Schedule: scrollable list (drag like the Hex page). The item whose start
// time has most recently passed is highlighted. Before the first start time
// nothing is highlighted; after the last one the last item stays highlighted.
// When the page opens, or the highlight moves, the list scrolls to it.
class SchedulePage : public BasePage {
  public:
    static constexpr int ITEM_COUNT = 15;
    // Start time in minutes since midnight, and the label shown
    const int startMin[ITEM_COUNT] = {
      // start
      7 * 60 + 0,
      // start
      10 * 60 + 45,
      // arrive
      11 * 60 + 30,
      //ceremony
      12 * 60 + 0,
      // drinks
      12 * 60 + 45,
      // breakfast
      14 * 60 + 30,
      // sppeches
      17 * 60 + 30,
      // cocktail hour
      18 * 60 + 30,
      // fight
      19 * 60 + 15,
      // cake cutting
      19 * 60 + 45,
      // first dance
      20 * 60 + 0,
      // band
      20 * 60 + 10,
      // food
      21 * 60 + 00,
      // music end
      23 * 60 + 30,
      // goodnight
      23 * 60 + 59
    };
    const char* items[ITEM_COUNT] = {
      "07:00 - Good morning",
      "10:45 - Groom's Ready",
      "11:30 - Guests Arrive",
      "12:00 - Ceremony",
      "12:45 - Drinks & Canapes",
      "14:30 - Wedding Breakfast",
      "17:30 - Speeches",
      "18:30 - Cocktail Hour",
      "19:15 - Scotish Boys Fight",
      "19:45 - Cake Cutting",
      "20:00 - First Dance",
      "20:10 - Live Band",
      "21:00 - Evening Food",
      "23:30 - Music-off Bar-close",
      "00:00 - Goodnight"
    };

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
      active = -2;
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
        lastTouch_ms = M5.millis();
      } else {
        dragLastY = -1;
      }
    }

    void releaseTouch() {
      dragLastY = -1;
    }

    void update() override {
      // Check the clock once a second
      uint32_t ms = M5.millis();
      if (active == -2 || ms - check_ms >= 1000) {
        check_ms = ms;
        int idx = currentIndex();
        if (idx != active) {
          bool first = (active == -2);
          active = idx;
          // Jump to the highlight on open; on a change only if the user
          // hasn't scrolled in the last 10 s
          if (active >= 0 && (first || ms - lastTouch_ms > 10000)) {
            scrollTo(active);
          }
          lastDrawnY = -1;  // force redraw
        }
      }
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
    int active = -2;          // -2 = not computed yet, -1 = before first item
    int lineH = 40;
    int scrollY = 0;
    int lastDrawnY = -1;
    float velocity = 0;
    int dragLastY = -1;
    uint32_t check_ms = 0;
    uint32_t lastTouch_ms = 0;

    M5Canvas canvas{&M5.Display};
    bool canvasReady = false;

    int currentIndex() {
      int now;
      if (M5.Rtc.isEnabled()) {
        auto t = M5.Rtc.getTime();
        now = t.hours * 60 + t.minutes;
      } else {
        now = (M5.millis() / 60000) % (24 * 60);  // no RTC: minutes since boot
      }
      int idx = -1;
      for (int i = 0; i < ITEM_COUNT; i++) {
        if (now >= startMin[i]) idx = i;
      }
      return idx;
    }

    int maxScroll() const {
      return max(0, ITEM_COUNT * lineH - (int)M5.Display.height());
    }

    void scrollBy(int dy) {
      scrollY = constrain(scrollY + dy, 0, maxScroll());
      if (scrollY == 0 || scrollY == maxScroll()) {
        velocity = 0;
      }
    }

    // Put item i in the middle of the screen
    void scrollTo(int i) {
      scrollY = constrain(i * lineH - ((int)M5.Display.height() - lineH) / 2, 0, maxScroll());
      velocity = 0;
    }

    template <typename T>
    void drawList(T& g) {
      int w = M5.Display.width(), h = M5.Display.height();
      g.fillRect(0, 0, w, h, TFT_BLACK);
      g.setFont(&fonts::DejaVu24);
      g.setTextDatum(textdatum_t::middle_center);
      for (int i = scrollY / lineH; i < ITEM_COUNT; i++) {
        int y = i * lineH - scrollY;
        if (y > h) {
          break;
        }
        bool on = (i == active);
        uint16_t bg = on ? EYE_RGB(255, 200, 0) : ((i & 1) ? EYE_RGB(20, 30, 50) : EYE_RGB(10, 15, 30));
        g.fillRect(0, y, w, lineH - 1, bg);
        g.setTextSize(1);
        float sz = min((lineH - 8) / (float)g.fontHeight(), (w - 16) / (float)g.textWidth(items[i]));
        g.setTextSize(min(1.0f, sz));
        g.setTextColor(on ? TFT_BLACK : TFT_WHITE);
        g.drawString(items[i], w / 2, y + lineH / 2);
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

#endif