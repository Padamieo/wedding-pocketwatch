// #include <M5UnitRCA.h>      /// UnitRCA を使う場合、これを追加する。
// #include <M5UnitLCD.h>      /// UnitLCD を使う場合、これを追加する。
// #include <M5UnitOLED.h>     /// UnitOLED を使う場合、これを追加する。
// #include <M5UnitGLASS.h>    /// UnitGLASS を使う場合、これを追加する。
// #include <M5AtomDisplay.h>  /// AtomDisplay を使う場合、これを追加する。

#include <M5Unified.h>

static void func_hello() {
  int w = M5.Display.width() >> 3;
  int h = M5.Display.height() >> 3;
  for (int i = 0; i < 128; ++i) {
    M5.Display.fillRect(
      rand() % M5.Display.width(),
      rand() % M5.Display.height(),
      w, h,
      (i&1) ? TFT_BLACK:TFT_WHITE);
  }
}

class Menu {
  public:
    struct menu_item_t {
      const char* title;
      void (*func)(void);
    };
    int menu_x = 2;
    int menu_y = 20;
    int menu_w = 120;
    int menu_h = 30;
    int menu_padding = 36;
    size_t cursor_index = 0;
    menu_item_t menus[2] = {
      { "Hello", func_hello },
      { "Clock", func_hello }
      // { "Hex", func_rect },
      // { "Photos", func_line },
      // { "Schedule", func_triangle },
      // { "arc", func_arc },
    };
    //size_t menu_count = 0;
    size_t menu_count = 2;

    // int myNum; // Attribute
    void init() { // Method
      M5.Display.fillScreen(TFT_NAVY);
      
      M5.Display.setCursor(menu_x, 0);
      M5.Display.print("MENU");

      for (size_t i = 0; i < menu_count; i++) {
        draw_menu(i, i == cursor_index);
      }
      // menus[2] 
      // constexpr const size_t menu_count = sizeof(menus) / sizeof(this.menus[0]);
      menu_count = sizeof(menus) / sizeof(menus[0]);
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
      M5.Speaker.tone(Hz, 100);
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
        M5.Speaker.tone(880, 150);
      }

      if (menus[cursor_index].func != nullptr) {
        // M5.Display.setClipRect(menu_x + menu_w, 0, M5.Display.width(), M5.Display.height());
        M5.Display.setClipRect(0, 0, M5.Display.width() - menu_w + 10, M5.Display.height());
        menus[cursor_index].func();
        M5.Display.clearClipRect();
      } else {
        // NOTE: for init
        menus[1].func();
      }
    }
};

Menu myObj; 

void setup(void) {
  auto cfg = M5.config();

  M5.begin(cfg);

  if (M5.Display.width() > M5.Display.height()) {
    M5.Display.setRotation(M5.Display.getRotation() ^ 1);
  }
  // M5.Display.setRotation(M5.Display.getRotation() ^ 3);

  M5.Display.startWrite();

  myObj.menu_w = M5.Display.width() >> 1;

  // if (myObj.menu_w < 70) {
  //   myObj.menu_y = 10;
  //   myObj.menu_w = (M5.Display.width() * 4) / 5;
  // } else {
    M5.Display.setFont(&fonts::DejaVu18);
    if (M5.Display.width() > 400) {
      M5.Display.setTextSize(2);
      myObj.menu_y *= 2;
      myObj.menu_w = M5.Display.width() >> 1;
    }
  // }

  static int x = M5.Lcd.width() / 2;

  myObj.menu_x = x - myObj.menu_w / 2;

  myObj.menu_padding = (M5.Display.height() - myObj.menu_y) / myObj.menu_count;
  myObj.menu_h = myObj.menu_padding - 2;

  /// このサンプルでは、startWriteをしたまま、対になるendWriteを使わないようにする。
  // M5.Display.startWrite();

  M5.Display.setEpdMode(epd_mode_t::epd_fastest);

  // M5.Display.fillScreen(TFT_NAVY);
  
  // M5.Display.setCursor(myObj.menu_x, 0);
  // M5.Display.print("MENU");

  // for (size_t i = 0; i < myObj.menu_count; i++) {
  //   myObj.draw_menu(i, i == myObj.cursor_index);
  // }

  // Create an object of MyClass
  myObj.init();  // Call the method

  //myObj.menu_count = sizeof(myObj.menus) / sizeof(myObj.menus[0]);

}

void loop(void) {
  /// ディスプレイがビジー状態でない場合のみ処理する。
  /// EPDでは画面の更新中はBusyとなるためここを通らない。
  if (!M5.Display.displayBusy()) {
    // 選択しているメニューが変更されていれば再描画。
    static size_t prev_index = 0;
    if (prev_index != myObj.cursor_index) {
      myObj.draw_menu(prev_index, false);
      myObj.draw_menu(myObj.cursor_index, true);
      prev_index = myObj.cursor_index;
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
    if ( ((size_t)detail.x - myObj.menu_x) < myObj.menu_w) {
      size_t index = (detail.y - myObj.menu_y) / myObj.menu_padding;
      if (index < myObj.menu_count) {
        if (detail.wasPressed()) {
          myObj.select_menu(index);
        } else
        if (index == myObj.cursor_index) {
          if (detail.wasClicked()) {
            myObj.exec_menu(false);
          }
          else
          if (detail.isHolding()) {
            myObj.exec_menu(true);
          }
        }
      }
    }
    //
  }

  //myObj.menus[cursor_index].func();

  switch (M5.getBoard()) {
  default:
    if (M5.BtnA.wasClicked()) {
      myObj.move_menu(false);
    }
    if (M5.BtnA.wasHold()) {
      myObj.exec_menu(false);
    }
    if (M5.BtnA.isHolding()) {
      myObj.exec_menu(true);
    }
    if (M5.BtnB.wasClicked()) {
      myObj.move_menu(true);
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