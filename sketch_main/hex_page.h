#ifndef HEX_PAGE_H
#define HEX_PAGE_H

// Define the hex page
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

#endif