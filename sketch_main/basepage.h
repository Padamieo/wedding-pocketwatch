#ifndef BASE_PAGE_H
#define BASE_PAGE_H

// Define the class for LED control
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

#endif