#ifndef HELLO_PAGE_H
#define HELLO_PAGE_H

// Define the hello page
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

#endif