#ifndef EYE_PAGE_H
#define EYE_PAGE_H

class EyePage : public BasePage {
  public:
    enum Effect {
      NORMAL, CAT, BLOOD, ZOMBIE, HEART, HYPNO, DOLLAR, BLOODSHOT, ROBOT, DEAD,
      SCARED, CUTE, STAR, DEMON, GOOGLY, CRYING, RAINBOW, SNAKE, ALIEN, SLEEPY,
      // Naruto
      TOAD, SHARINGAN1, SHARINGAN2, SHARINGAN3, MANGEKYO_ITACHI, MANGEKYO_SASUKE,
      ETERNAL_MANGEKYO, RINNEGAN, RINNE_SHARINGAN, BYAKUGAN,
      // Anime
      SPARKLE,
      EFFECT_COUNT
    };

    int cx, cy, eyeR, pupilR;
    int effect = NORMAL;
    uint32_t phase = 0;      // animation frame counter
    uint32_t anim_ms = 0;
    bool closed = false;
    bool dirty = true;       // frame needs to be redrawn and pushed
    uint32_t next_ms = 0;
    int blinksLeft = 0;      // >0 while doing a double blink

    // "Looking around": pupil offset eases toward a random target
    float lookX = 0, lookY = 0, lookTX = 0, lookTY = 0;
    uint32_t look_ms = 0;

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
      next_ms = M5.millis() + openTime();
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
      next_ms = M5.millis() + openTime();
      lookX = lookY = lookTX = lookTY = 0;
      dirty = true;
    }

    void update() override {
      uint32_t ms = M5.millis();
      if (ms >= next_ms) {
        // Blink every 2.5..7 s, closed for 100..200 ms,
        // with a ~20% chance of a quick double blink
        closed = !closed;
        if (closed) {
          next_ms = ms + 100 + esp_random() % 100;
        } else if (blinksLeft > 0) {
          blinksLeft--;
          next_ms = ms + 120;
        } else {
          next_ms = ms + openTime();
          if (esp_random() % 5 == 0) blinksLeft = 1;
        }
        dirty = true;
      }
      if ((isAnimated() || looksAround()) && ms - anim_ms >= 40) {
        anim_ms = ms;
        phase++;
        if (looksAround()) updateLook(ms);
        dirty = true;
      }
      present();
    }

  private:
    static uint32_t openTime() { return 2500 + esp_random() % 4500; }

    bool isAnimated() const {
      return effect == BLOOD || effect == HYPNO || effect == CRYING || effect == RAINBOW
          || effect == DEMON || effect == ROBOT || effect == GOOGLY || effect == SPARKLE
          || (effect >= SHARINGAN1 && effect <= RINNE_SHARINGAN);
    }

    // Effects whose pupil glances around
    bool looksAround() const {
      switch (effect) {
        case NORMAL: case CAT: case CUTE: case SNAKE: case BLOODSHOT: case HEART:
        case SCARED: case TOAD: case SHARINGAN1: case SHARINGAN2: case SHARINGAN3:
        case SPARKLE: case ZOMBIE:
          return true;
        default:
          return false;
      }
    }

    // Pick a new gaze point every 0.6..2.5 s (often back to centre),
    // then ease toward it so the motion looks like a glance
    void updateLook(uint32_t ms) {
      if (ms >= look_ms) {
        float range = (eyeR - pupilR) * 0.55f;
        if (esp_random() % 3 == 0) {
          lookTX = lookTY = 0;
        } else {
          float a = (esp_random() % 360) * M_PI / 180;
          float d = range * (0.5f + (esp_random() % 50) / 100.0f);
          lookTX = cosf(a) * d;
          lookTY = sinf(a) * d * 0.7f;
        }
        look_ms = ms + 600 + esp_random() % 1900;
      }
      lookX += (lookTX - lookX) * 0.3f;
      lookY += (lookTY - lookY) * 0.3f;
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

    // Star rotated by rot radians
    template <typename T>
    void starRot(T& g, int x, int y, int R, float rot, uint16_t c) {
      int r = R * 2 / 5;
      int px[10], py[10];
      for (int i = 0; i < 10; i++) {
        float a = rot - M_PI / 2 + i * M_PI / 5;
        int rr = (i & 1) ? r : R;
        px[i] = x + (int)(cosf(a) * rr);
        py[i] = y + (int)(sinf(a) * rr);
      }
      for (int i = 0; i < 10; i++) {
        int j = (i + 1) % 10;
        g.fillTriangle(x, y, px[i], py[i], px[j], py[j], c);
      }
    }

    // One tomoe (comma) on an orbit of radius orb around (x, y) at angle a
    template <typename T>
    void tomoe(T& g, int x, int y, int orb, float a, int r, uint16_t c) {
      int tx = x + (int)(cosf(a) * orb), ty = y + (int)(sinf(a) * orb);
      g.fillCircle(tx, ty, r, c);
      // Tail trails backwards along the orbit
      float b = a - 0.55f;
      int ex = x + (int)(cosf(b) * (orb + r * 0.4f)), ey = y + (int)(sinf(b) * (orb + r * 0.4f));
      float n = a + M_PI / 2;
      g.fillTriangle(tx + (int)(cosf(a) * r), ty + (int)(sinf(a) * r),
                     tx - (int)(cosf(n) * r * 0.3f), ty - (int)(sinf(n) * r * 0.3f), ex, ey, c);
    }

    // Red iris with black outer ring; returns iris radius
    template <typename T>
    int sharinganBase(T& g, int x, int y) {
      int ir = eyeR * 3 / 4;
      g.fillCircle(x, y, ir, EYE_RGB(200, 0, 0));
      for (int k = 0; k < 3; k++) g.drawCircle(x, y, ir - k, TFT_BLACK);
      return ir;
    }

    // Classic Sharingan with 1..3 slowly spinning tomoe
    template <typename T>
    void sharingan(T& g, int x, int y, int count) {
      int ir = sharinganBase(g, x, y);
      int orb = ir * 11 / 20;
      g.drawCircle(x, y, orb, EYE_RGB(90, 0, 0));
      g.drawCircle(x, y, orb + 1, EYE_RGB(90, 0, 0));
      float rot = phase * 0.06f;
      for (int k = 0; k < count; k++) {
        tomoe(g, x, y, orb, rot + k * 2 * M_PI / count, ir / 7, TFT_BLACK);
      }
      g.fillCircle(x, y, ir / 5, TFT_BLACK);
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
      int px = ex + (int)lookX, py = ey + (int)lookY;  // gaze-shifted centre
      switch (effect) {
        case NORMAL:
          g.fillCircle(ex, ey, R, TFT_WHITE);
          basicPupil(g, px, py, P, TFT_BLUE);
          break;

        case CAT:
          g.fillCircle(ex, ey, R, EYE_RGB(200, 220, 40));
          g.fillCircle(px, py, R * 3 / 4, EYE_RGB(230, 170, 20));
          g.fillEllipse(px, py, P / 4, R * 7 / 10, TFT_BLACK);
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
          g.fillCircle(px + R / 8, py + R / 10, P, EYE_RGB(215, 215, 200));
          g.fillCircle(px + R / 8, py + R / 10, P / 2, EYE_RGB(185, 185, 175));
          break;

        case HEART:
          g.fillCircle(ex, ey, R, TFT_WHITE);
          heart(g, px, py, P + P / 3, EYE_RGB(230, 0, 80));
          g.fillCircle(px - P / 2, py - P / 2, max(2, P / 6), TFT_WHITE);
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
          basicPupil(g, px, py, P, EYE_RGB(110, 70, 30));
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
          // Thick X with rounded ends
          g.drawWideLine(ex - P * 3 / 2, ey - P * 3 / 2, ex + P * 3 / 2, ey + P * 3 / 2, max(4.0f, P * 0.45f), TFT_BLACK);
          g.drawWideLine(ex + P * 3 / 2, ey - P * 3 / 2, ex - P * 3 / 2, ey + P * 3 / 2, max(4.0f, P * 0.45f), TFT_BLACK);
          break;

        case SCARED:
          g.fillCircle(ex, ey, R, TFT_WHITE);
          g.fillCircle(px, py, P / 4, TFT_BLACK);
          break;

        case CUTE:
          g.fillCircle(ex, ey, R, TFT_WHITE);
          g.fillCircle((px + ex) / 2, (py + ey) / 2, R * 3 / 4, EYE_RGB(90, 50, 20));
          g.fillCircle((px + ex) / 2, (py + ey) / 2, R / 2, TFT_BLACK);
          g.fillCircle((px + ex) / 2 - R / 4, (py + ey) / 2 - R / 4, R / 7, TFT_WHITE);
          g.fillCircle((px + ex) / 2 + R / 5, (py + ey) / 2 + R / 5, R / 14, TFT_WHITE);
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
          g.fillCircle(px, py, R * 2 / 3, EYE_RGB(150, 210, 30));
          g.fillEllipse(px, py, P / 8 + 1, R * 3 / 5, TFT_BLACK);
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

        case TOAD:  // Sage Mode: orange pigment, gold iris, horizontal bar pupil
          g.fillCircle(ex, ey, R, EYE_RGB(230, 110, 20));
          g.fillCircle(ex, ey, R - R / 8, EYE_RGB(255, 210, 40));
          g.fillCircle(px, py, R * 3 / 5, EYE_RGB(240, 180, 0));
          g.fillRoundRect(px - R / 2, py - P / 4, R, P / 2, P / 4, TFT_BLACK);
          g.fillCircle(px - R / 4, py - R / 4, max(2, R / 12), TFT_WHITE);
          break;

        case SHARINGAN1:
        case SHARINGAN2:
        case SHARINGAN3:
          g.fillCircle(ex, ey, R, TFT_WHITE);
          sharingan(g, (px + ex * 2) / 3, (py + ey * 2) / 3, effect - SHARINGAN1 + 1);
          break;

        case MANGEKYO_ITACHI: {  // three-bladed pinwheel
          g.fillCircle(ex, ey, R, TFT_WHITE);
          int ir = sharinganBase(g, ex, ey);
          float rot = phase * 0.04f;
          for (int k = 0; k < 3; k++) {
            float a = rot + k * 2 * M_PI / 3;
            int x1 = ex + (int)(cosf(a) * ir * 0.95f), y1 = ey + (int)(sinf(a) * ir * 0.95f);
            int x2 = ex + (int)(cosf(a + 0.9f) * ir * 0.45f), y2 = ey + (int)(sinf(a + 0.9f) * ir * 0.45f);
            int x3 = ex + (int)(cosf(a - 0.5f) * ir * 0.3f), y3 = ey + (int)(sinf(a - 0.5f) * ir * 0.3f);
            g.fillTriangle(ex, ey, x1, y1, x2, y2, TFT_BLACK);
            g.fillTriangle(ex, ey, x1, y1, x3, y3, TFT_BLACK);
          }
          g.fillCircle(ex, ey, ir * 3 / 10, TFT_BLACK);
          g.fillCircle(ex, ey, ir / 8, EYE_RGB(200, 0, 0));
          break;
        }

        case MANGEKYO_SASUKE: {  // black ring with three curved points
          g.fillCircle(ex, ey, R, TFT_WHITE);
          int ir = sharinganBase(g, ex, ey);
          float rot = phase * 0.04f;
          g.fillArc(ex, ey, ir / 2, ir * 2 / 5, 0, 360, TFT_BLACK);
          for (int k = 0; k < 3; k++) {
            float a = rot + k * 2 * M_PI / 3;
            int tx = ex + (int)(cosf(a) * ir * 0.95f), ty = ey + (int)(sinf(a) * ir * 0.95f);
            int bx1 = ex + (int)(cosf(a - 0.6f) * ir * 0.45f), by1 = ey + (int)(sinf(a - 0.6f) * ir * 0.45f);
            int bx2 = ex + (int)(cosf(a + 0.3f) * ir * 0.45f), by2 = ey + (int)(sinf(a + 0.3f) * ir * 0.45f);
            g.fillTriangle(tx, ty, bx1, by1, bx2, by2, TFT_BLACK);
          }
          g.fillCircle(ex, ey, ir / 6, TFT_BLACK);
          break;
        }

        case ETERNAL_MANGEKYO: {  // hexagram with counter-rotating blades
          g.fillCircle(ex, ey, R, TFT_WHITE);
          int ir = sharinganBase(g, ex, ey);
          float rot = phase * 0.03f;
          for (int t = 0; t < 2; t++) {
            float o = rot + t * M_PI / 3;
            int xs[3], ys[3];
            for (int k = 0; k < 3; k++) {
              float a = o + k * 2 * M_PI / 3;
              xs[k] = ex + (int)(cosf(a) * ir * 0.9f);
              ys[k] = ey + (int)(sinf(a) * ir * 0.9f);
            }
            g.fillTriangle(xs[0], ys[0], xs[1], ys[1], xs[2], ys[2], TFT_BLACK);
          }
          g.fillCircle(ex, ey, ir * 9 / 20, EYE_RGB(200, 0, 0));
          for (int k = 0; k < 3; k++) {
            float a = -rot * 2 + k * 2 * M_PI / 3;
            int x1 = ex + (int)(cosf(a) * ir * 0.45f), y1 = ey + (int)(sinf(a) * ir * 0.45f);
            int x2 = ex + (int)(cosf(a + 1.2f) * ir * 0.25f), y2 = ey + (int)(sinf(a + 1.2f) * ir * 0.25f);
            g.fillTriangle(ex, ey, x1, y1, x2, y2, TFT_BLACK);
          }
          g.fillCircle(ex, ey, ir / 7, TFT_BLACK);
          break;
        }

        case RINNEGAN: {  // lavender with concentric ripples, pulsing slowly
          g.fillCircle(ex, ey, R, EYE_RGB(185, 160, 220));
          const int rings = 6;
          int pulse = (int)(sinf(phase * 0.08f) * (R / 30.0f));
          for (int k = 1; k <= rings; k++) {
            int rr = k * (R - 4) / (rings + 1) + pulse;
            g.drawCircle(ex, ey, rr, EYE_RGB(40, 20, 70));
            g.drawCircle(ex, ey, rr + 1, EYE_RGB(40, 20, 70));
          }
          g.fillCircle(ex, ey, R / 10, EYE_RGB(40, 20, 70));
          break;
        }

        case RINNE_SHARINGAN: {  // red ripples with 9 tomoe on 3 rings
          g.fillCircle(ex, ey, R, EYE_RGB(210, 0, 20));
          for (int k = 1; k <= 4; k++) {
            int rr = k * (R - 4) / 4;
            g.drawCircle(ex, ey, rr, TFT_BLACK);
            g.drawCircle(ex, ey, rr - 1, TFT_BLACK);
          }
          float rot = phase * 0.05f;
          for (int ring = 1; ring <= 3; ring++) {
            int orb = ring * (R - 4) / 4;
            float dir = (ring & 1) ? 1.0f : -1.0f;  // alternate spin direction
            for (int k = 0; k < 3; k++) {
              tomoe(g, ex, ey, orb, rot * dir + k * 2 * M_PI / 3 + ring, max(3, R / 14), TFT_BLACK);
            }
          }
          g.fillCircle(ex, ey, R / 10, TFT_BLACK);
          break;
        }

        case BYAKUGAN:  // pale, pupil-less, bulging veins at the sides
          g.fillCircle(ex, ey, R, EYE_RGB(235, 230, 245));
          g.fillCircle(ex, ey, R * 3 / 5, EYE_RGB(215, 210, 235));
          g.drawCircle(ex, ey, R * 3 / 5, EYE_RGB(190, 185, 215));
          for (int s = -1; s <= 1; s += 2) {
            for (int k = 0; k < 4; k++) {
              int x0 = ex + s * (R - 2), y0 = ey - R / 3 + k * R / 5;
              g.drawWideLine(x0, y0, x0 - s * R / 4, y0 + (k - 2) * R / 12, 2.0f, EYE_RGB(150, 140, 190));
            }
          }
          break;

        case SPARKLE: {  // pink anime eye with spinning sparkle stars
          g.fillCircle(ex, ey, R, TFT_WHITE);
          int sx = (px + ex * 2) / 3, sy = (py + ey * 2) / 3;
          int ir = R * 3 / 4;
          g.fillCircle(sx, sy, ir, EYE_RGB(200, 40, 130));
          g.fillCircle(sx, sy + ir / 6, ir * 5 / 6, EYE_RGB(255, 100, 180));
          g.fillCircle(sx, sy + ir / 3, ir / 2, EYE_RGB(255, 170, 215));
          g.fillCircle(sx, sy, ir / 3, EYE_RGB(110, 0, 70));
          float rot = phase * 0.15f;
          starRot(g, sx - ir / 3, sy - ir / 3, ir / 3, rot, TFT_WHITE);
          starRot(g, sx + ir / 3, sy + ir / 4, ir / 6, -rot * 1.5f, TFT_WHITE);
          // Twinkling small stars orbiting inside the iris
          for (int k = 0; k < 4; k++) {
            float a = rot * 0.5f + k * M_PI / 2;
            int tw = max(2, (int)((ir / 8) * (0.6f + 0.4f * sinf(phase * 0.3f + k))));
            starRot(g, sx + (int)(cosf(a) * ir * 0.7f), sy + (int)(sinf(a) * ir * 0.7f), tw, rot * 2, EYE_RGB(255, 230, 245));
          }
          break;
        }
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

#endif