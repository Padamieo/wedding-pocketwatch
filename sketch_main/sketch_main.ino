#define NTP_TIMEZONE "UTC-0"
#define NTP_SERVER1  "0.pool.ntp.org"
#define NTP_SERVER2  "1.pool.ntp.org"
#define NTP_SERVER3  "2.pool.ntp.org"

#include <Arduino.h>
#include <M5Unified.h>
#include <WiFi.h>
#include <esp_sntp.h>
#include <sntp.h>
#include "base_page.h"
#include "hello_page.h"

#define EYE_RGB(r, g, b) ((uint16_t)((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3)))

#include "hex_page.h"
#include "eye_page.h"
#include "game_page.h"
#include "schedule_page.h"

void updateContentPage(size_t cursor_index); 

const char *ssid = "Adam";
const char *password = "pancakes";

class ClockPage : public BasePage {
  public:
    bool analog = false;
    int lastSecond = -1;
    bool internetSynced = false;

    // Off-screen buffer for the analog face (falls back to direct drawing)
    M5Canvas canvas{&M5.Display};
    bool canvasReady = false;
    int faceR = 0;

    void initInternet() {
      // move into its own thing?
      WiFi.mode(WIFI_STA);
      M5.Display.setCursor(100, 100);
      M5.Display.println("WiFi mode set to STA");
      WiFi.begin(ssid, password);
      
      while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        M5.Display.print(".");
      }

      configTzTime(NTP_TIMEZONE, NTP_SERVER1, NTP_SERVER2, NTP_SERVER3);

      while (sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED) {
        Serial.print('.');
        delay(1000);
      }

      time_t t = time(nullptr) + 1;  // Advance one second.
      while (t > time(nullptr));     /// Synchronization in seconds
      M5.Rtc.setDateTime(gmtime(&t));
      //internetSynced = true;

    }

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

      // initInternet();
    }

    void toggleMode() {
      analog = !analog;
      initialized = false;  // full redraw on next render()
    }

    void init() override {
      if (initialized) {
        return;
      }
      M5.Display.fillScreen(TFT_MAROON);
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
      if (!internetSynced) {
        M5.Log.println("update");
        
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
      float timeScale = (w * 0.85f) / M5.Display.textWidth("88:88");
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
    int menu_y = 10;
    int menu_w = 120;
    int menu_h = 30;
    int menu_padding = 25;
    size_t cursor_index = 0;
    menu_item_t menus[6] = {
      { "Hello", updateContentPage },
      { "Clock", updateContentPage },
      { "Hex", updateContentPage },
      // { "Photos", updateContentPage },
      { "Schedule", updateContentPage },
      { "Eye", updateContentPage },
      { "Game", updateContentPage }
    };
    size_t menu_count = 6;

    // initial screen setup
    void init() {
      if (initialized) {
        return;
      }
      
      int w = M5.Display.width() >> 3;
      int h = M5.Display.height() >> 3;
      for (int i = 0; i < 128; ++i) {
        uint16_t c = rand();
        M5.Display.fillRect(
          rand() % M5.Display.width(),
          rand() % M5.Display.height(),
          w, h, c
        );
        M5.delay(1);
      }
      
      M5.Display.setFont(&fonts::DejaVu18);
      M5.Display.setTextSize(2);

      static int x = M5.Lcd.width() / 2;
      menu_w = M5.Display.width() >> 1;
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
      if (holding == false) {
        sound && M5.Speaker.tone(880, 150);
      }

      if (menus[cursor_index].func != nullptr) {
        menus[cursor_index].func(cursor_index);
        visible = false;
        initialized = false;
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

    void onTouch(auto detail) {
      if ( ((size_t)detail.x - menu_x) < menu_w) {
        size_t index = (detail.y - menu_y) / menu_padding;
        if (index < menu_count) {
          if (detail.wasPressed()) {
            select_menu(index);
          } else if (index == cursor_index) {
            if (detail.wasClicked()) {
              exec_menu(false);
            } else if (detail.isHolding()) {
              exec_menu(true);
            }
          }
        }
      }
    }
};

// setup classes
BasePage* contentPage;
// BasePage basePage;

HelloPage helloPage;
ClockPage clockPage;
HexPage hexPage;
SchedulePage schedulePage;
EyePage eyePage;
GamePage gamePage;

Menu mainMenu;

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
  // if(cursor_index == 3) {
  //   contentPage = &schedulePage;
  // }
  if(cursor_index == 3) {
    contentPage = &schedulePage;
  }
  if(cursor_index == 4) {
    contentPage = &eyePage;
  }
  if(cursor_index == 5) {
    contentPage = &gamePage;
  }
  contentPage->initialized = false;
  // M5.Power.setVibration(100);
}

// main startup function
void setup(void) {
  
  new(&mainMenu) Menu(); // reconstruct

  // contentPage = &clockPage;
  contentPage = &eyePage;

  auto cfg = M5.config();
  M5.begin(cfg);
  Serial.begin(115200);

  // if (M5.Display.width() > M5.Display.height()) {
  //   M5.Display.setRotation(M5.Display.getRotation() ^ 1);
  // }
  // M5.Display.setRotation(M5.Display.getRotation() ^ 3);

  /// このサンプルでは、startWriteをしたまま、対になるendWriteを使わないようにする。
  // M5.Display.startWrite();

  M5.Display.setEpdMode(epd_mode_t::epd_fastest);

  ClockPage::initTime();

  contentPage->init();
}

// main update function
void loop(void) {
  if (!M5.Display.displayBusy()) {
    if(mainMenu.visible) {
      mainMenu.update();
    } else {
      contentPage->render();
    }
    M5.Display.display();
  }
  {
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
  if (touch_count == 0 && contentPage == &hexPage) {
    hexPage.releaseTouch();  // finger lifted: end the drag so the fling starts
    schedulePage.releaseTouch();
  }
  for (size_t i = 0; i < touch_count; i++) {
    auto detail = M5.Touch.getDetail(i);
    if (!mainMenu.visible){
      // Tap the clock page to switch between digital and analog
      if (contentPage == &clockPage && detail.wasPressed()) {
        clockPage.toggleMode();
      }
      // Drag on the hex page to scroll the list
      if (contentPage == &hexPage) {
        hexPage.onTouch(detail);
      }
      // Drag on the schedule page to scroll the list
      if (contentPage == &schedulePage) {
        schedulePage.onTouch(detail);
      }
      // Tap the eye page to switch to a random eye effect
      if (contentPage == &eyePage && detail.wasPressed()) {
        eyePage.randomEffect();
      }
      // Junior / Senior choice and "back" tap on the game page
      if (contentPage == &gamePage && detail.wasPressed()) {
        gamePage.onTap(detail.x);
      }
    }
    if(mainMenu.visible) {
      mainMenu.onTouch(detail);
    }
  }

  switch (M5.getBoard()) {
  default:
    if (M5.BtnA.wasClicked()) {
      if(mainMenu.visible) {
        mainMenu.move_menu(false);
      } else {
        if (contentPage == &gamePage) {
          gamePage.BtnA();
        }
        if (contentPage == &eyePage) {
          eyePage.BtnA();
        }
      }
    }
    if (M5.BtnA.wasHold()) {
      if(mainMenu.visible) {
        mainMenu.exec_menu(false);
      } else {
        if (contentPage == &eyePage) {
          eyePage.BtnAhold();
        }
      }
    }
    if (M5.BtnA.isHolding() && M5.BtnB.isHolding()) {
      if(!mainMenu.visible) {
        mainMenu.visible = true;
        mainMenu.init();
      }
    }
    if (M5.BtnB.wasClicked()) {
      if(mainMenu.visible) {
        mainMenu.move_menu(true);
      } else {
        if (contentPage == &gamePage) {
          gamePage.BtnB();
        }
        if (contentPage == &eyePage) {
          eyePage.BtnB();
        }
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