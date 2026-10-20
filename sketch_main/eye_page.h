#ifndef EYE_PAGE_H
#define EYE_PAGE_H

// Define the hex page
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

#endif