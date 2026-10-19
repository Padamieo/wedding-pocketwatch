// #include <M5UnitRCA.h>      /// UnitRCA を使う場合、これを追加する。
// #include <M5UnitLCD.h>      /// UnitLCD を使う場合、これを追加する。
// #include <M5UnitOLED.h>     /// UnitOLED を使う場合、これを追加する。
// #include <M5UnitGLASS.h>    /// UnitGLASS を使う場合、これを追加する。
// #include <M5AtomDisplay.h>  /// AtomDisplay を使う場合、これを追加する。

#include <M5Unified.h>

static void func_hello(size_t cursor_index) {
  // M5.Display.clear();
  int w = M5.Display.width() >> 3;
  int h = M5.Display.height() >> 3;
  for (int i = 0; i < 128; ++i) {
    uint16_t c = rand();
    M5.Display.fillRect(
      rand() % M5.Display.width(),
      rand() % M5.Display.height(),
      w, h,
      c
    );
  }
}

class BasePage {
  public:
    bool initialized = false;

    virtual void render() {
      if (initialized) {
        update();
      } else {
        init();
      }
    }

    virtual void init() {
      if (initialized) {
        return;
      }
      M5.Display.fillScreen(TFT_DARKGREY);
      M5.Display.setCursor(100, 100);
      M5.Display.print("fallback init");
      initialized = true;
    }

    virtual void update() {
      // M5.Display.fillScreen(TFT_DARKGREY);
      // M5.Display.setCursor(100, 100);
      // M5.Display.print("fallback update");
    }
};

class HelloPage : public BasePage {
  public:
    void init() override {
      if (initialized) {
        return;
      }
      M5.Display.fillScreen(TFT_DARKGREEN);
      M5.Display.setCursor(100, 100);
      M5.Display.print("HelloPage init");
      initialized = true;
    }

    void update() override {
      // M5.Display.fillScreen(TFT_DARKGREEN);
      // M5.Display.setCursor(100, 100);
      // M5.Display.print("HelloPage update");
    }
};

class ClockPage : public BasePage {
  public:
    void init() override {
      if (initialized) {
        return;
      }
      M5.Display.fillScreen(TFT_MAROON);
      M5.Display.setCursor(100, 100);
      M5.Display.print("CLOCK init");
      initialized = true;
    }

    void update() override {
      // M5.Display.fillScreen(TFT_MAROON);
      // M5.Display.setCursor(100, 100);
      // M5.Display.print("CLOCK update");
    }
};

class HexPage : public BasePage {
  public:
    void init() override {
      if (initialized) {
        return;
      }
      M5.Display.fillScreen(TFT_DARKCYAN);
      M5.Display.setCursor(100, 100);
      M5.Display.print("HexPage init");
      initialized = true;
    }

    void update() override {
      // M5.Display.fillScreen(TFT_DARKCYAN);
      // M5.Display.setCursor(100, 100);
      // M5.Display.print("HexPage update");
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
    menu_item_t menus[5] = {
      { "Hello", updateContentPage },
      { "Clock", updateContentPage },
      { "Hex", updateContentPage },
      { "Photos", updateContentPage },
      { "Schedule", func_hello }
      // { "arc", func_arc },
    };
    //size_t menu_count = 0;
    size_t menu_count = 5;

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
BasePage basePage;

Menu myObj;

static void updateContentPage(size_t cursor_index) {
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
    contentPage = &basePage;
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
  for (size_t i = 0; i < touch_count; i++) {
    auto detail = M5.Touch.getDetail(i);
    //
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