// #include <M5Unified.h>
// #include "BasePage.h"

// BasePage::render() {
//   if (initialized) {
//     update();
//   } else {
//     init();
//   }
// }

// BasePage::init() {
//   if (initialized) {
//     return;
//   }
//   M5.Display.fillScreen(TFT_DARKGREY);
//   M5.Display.setCursor(100, 100);
//   M5.Display.print("fallback init");
//   initialized = true;
// }

// BasePage::update() {
//   // M5.Display.fillScreen(TFT_DARKGREY);
//   // M5.Display.setCursor(100, 100);
//   // M5.Display.print("fallback update");
// }