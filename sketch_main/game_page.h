#ifndef GAME_PAGE_H
#define GAME_PAGE_H

// Junior / Senior picker.
//  - Left half "Junior": a random animal (30), drawn full screen with code.
//  - Right half "Senior": a random famous person (100) or historic item (100).
// Tap the result to go back to the choice screen.
class GamePage : public BasePage {
  public:
    enum Mode { CHOOSE, RESULT };
    Mode mode = CHOOSE;

    static constexpr int ANIMAL_COUNT = 30;
    static constexpr int PERSON_COUNT = 100;
    static constexpr int ITEM_COUNT = 100;

    const char* animalNames[ANIMAL_COUNT] = {
      "Cat", "Dog", "Pig", "Cow", "Sheep", "Frog",
      "Fish", "Owl", "Penguin", "Mouse", "Rabbit", "Bear",
      "Panda", "Lion", "Tiger", "Fox", "Chick", "Duck",
      "Elephant", "Giraffe", "Monkey", "Koala", "Snail", "Ladybug",
      "Bee", "Butterfly", "Turtle", "Snake", "Octopus", "Whale"
    };
    const char* persons[PERSON_COUNT] = {
      "Albert Einstein", "Leonardo da Vinci", "Marie Curie", "Isaac Newton",
      "Cleopatra", "Julius Caesar", "Napoleon", "Mahatma Gandhi",
      "Nelson Mandela", "Martin Luther King", "Abraham Lincoln", "George Washington",
      "Winston Churchill", "Queen Victoria", "William Shakespeare", "Mozart",
      "Beethoven", "Pablo Picasso", "Vincent van Gogh", "Rembrandt",
      "Charles Darwin", "Galileo Galilei", "Nikola Tesla", "Thomas Edison",
      "Christopher Columbus", "Marco Polo", "Neil Armstrong", "Amelia Earhart",
      "Joan of Arc", "Genghis Khan", "Alexander the Great", "Confucius",
      "Aristotle", "Plato", "Socrates", "Frida Kahlo",
      "Charlie Chaplin", "Elvis Presley", "Michael Jackson", "Walt Disney",
      "Steve Jobs", "Florence Nightingale", "Anne Frank", "Queen Elizabeth II",
      "Henry VIII", "Tutankhamun", "Ada Lovelace", "Alan Turing",
      "Bill Gates", "Rosa Parks", "Mother Teresa", "Charles Dickens",
      "Jane Austen", "Mark Twain", "Johann Sebastian Bach", "Michelangelo",
      "Claude Monet", "Salvador Dali", "Andy Warhol", "Marilyn Monroe",
      "Audrey Hepburn", "John Lennon", "Freddie Mercury", "Bob Marley",
      "Muhammad Ali", "Pele", "Usain Bolt", "Serena Williams",
      "Boudica", "Yuri Gagarin", "Stephen Hawking", "Sigmund Freud",
      "Louis Pasteur", "Alexander Fleming", "Ferdinand Magellan", "James Cook",
      "Vasco da Gama", "Hannibal", "Charlemagne", "Augustus",
      "Nefertiti", "Ramses II", "Buddha", "Elizabeth I",
      "Catherine the Great", "Peter the Great", "John F. Kennedy", "Franklin D. Roosevelt",
      "Barack Obama", "Dalai Lama", "Harriet Tubman", "Benjamin Franklin",
      "Thomas Jefferson", "Erwin Schrödinger", "Agatha Christie", "J.R.R. Tolkien",
      "Homer", "Nicolaus Copernicus", "Archimedes", "Pythagoras"
    };
    const char* items[ITEM_COUNT] = {
      "Mona Lisa", "Great Pyramid", "Eiffel Tower", "Great Wall of China",
      "Colosseum", "Stonehenge", "Statue of Liberty", "Taj Mahal",
      "Rosetta Stone", "Titanic", "Terracotta Army", "Big Ben",
      "Parthenon", "Machu Picchu", "Leaning Tower of Pisa", "Great Sphinx",
      "Berlin Wall", "Magna Carta", "Printing Press", "Telephone",
      "Light Bulb", "Steam Engine", "Model T Ford", "Apollo 11",
      "Hubble Telescope", "Sputnik", "Gutenberg Bible", "Dead Sea Scrolls",
      "Venus de Milo", "Statue of David", "The Night Watch", "The Scream",
      "Sistine Chapel", "Notre-Dame", "Tower of London", "Golden Gate Bridge",
      "Sydney Opera House", "Mount Rushmore", "Easter Island Heads", "Petra",
      "Angkor Wat", "Chichen Itza", "Trojan Horse", "Viking Longship",
      "Samurai Sword", "Crown Jewels", "Hope Diamond", "Enigma Machine",
      "Wright Flyer", "Compass", "Abacus", "Sundial",
      "Astrolabe", "Hourglass", "Microscope", "Gramophone",
      "Typewriter", "Penny Black", "ENIAC Computer", "The Wheel",
      "Windmill", "Spinning Jenny", "Steam Locomotive", "Hindenburg",
      "Concorde", "Space Shuttle", "Moon Rock", "Liberty Bell",
      "Declaration of Independence", "Bayeux Tapestry", "Book of Kells", "Girl with a Pearl Earring",
      "The Starry Night", "The Last Supper", "Birth of Venus", "Guernica",
      "Hanging Gardens", "Lighthouse of Alexandria", "Colossus of Rhodes", "Hagia Sophia",
      "Forbidden City", "Kremlin", "Brandenburg Gate", "Arc de Triomphe",
      "Neuschwanstein", "Alhambra", "Acropolis", "Pompeii",
      "Tutankhamun's Mask", "Mayflower", "HMS Victory", "Santa Maria",
      "Rubik's Cube", "Sony Walkman", "Polaroid Camera", "First iPhone",
      "Atari 2600", "Commodore 64", "Gameboy", "Morse Telegraph"
    };

    void init() override {
      if (initialized) {
        return;
      }
      mode = CHOOSE;
      drawChoice();
      initialized = true;
    }

    void update() override {}

    void randomSenior() {
      int n = pickRandom(PERSON_COUNT + ITEM_COUNT, lastSenior);
      if (n < PERSON_COUNT) {
        showText("Famous person", persons[n], EYE_RGB(110, 20, 90));
      } else {
        showText("Historic item", items[n - PERSON_COUNT], EYE_RGB(20, 50, 120));
      }
    }

    // Called from loop() on a new screen press while this page is showing
    void onTap(int x) {
      if (mode == CHOOSE) {
        mode = RESULT;
        if (x < (int)M5.Display.width() / 2) {
          drawAnimal(pickRandom(ANIMAL_COUNT, lastAnimal));
        } else {
          randomSenior();
        }
      } else {
        mode = CHOOSE;
        drawChoice();
      }
    }

    void BtnA(){
      if (mode == CHOOSE) {
        mode = RESULT;
        drawAnimal(pickRandom(ANIMAL_COUNT, lastAnimal));
      } else {
        mode = CHOOSE;
        drawChoice();
      }
    }

    void BtnB() {
      if (mode == CHOOSE) {
        mode = RESULT;
        randomSenior();
      } else {
        mode = CHOOSE;
        drawChoice();
      }
    }

  private:
    int lastAnimal = -1, lastSenior = -1;
    int cx = 0, cy = 0, u = 1;  // drawing centre and unit size for animals

    static int pickRandom(int count, int& last) {
      int n;
      do {
        n = esp_random() % count;  // hardware RNG, different every boot
      } while (n == last && count > 1);
      last = n;
      return n;
    }

    void drawChoice() {
      int w = M5.Display.width(), h = M5.Display.height();
      M5.Display.fillRect(0, 0, w / 2, h, EYE_RGB(0, 150, 70));
      M5.Display.fillRect(w / 2, 0, w - w / 2, h, EYE_RGB(150, 30, 120));
      M5.Display.fillRect(w / 2 - 1, 0, 2, h, TFT_WHITE);
      M5.Display.setFont(&fonts::DejaVu24);
      M5.Display.setTextDatum(textdatum_t::middle_center);
      M5.Display.setTextSize(1);
      // Fit the longer word into half the screen
      float sz = min(2.0f, (w / 2 - 8) / (float)M5.Display.textWidth("Senior"));
      M5.Display.setTextSize(sz);
      M5.Display.setTextColor(TFT_WHITE);
      M5.Display.drawString("Junior", w / 4, h / 2);
      M5.Display.drawString("Senior", w * 3 / 4, h / 2);
    }

    // Big centred text with a small heading, wrapped onto two lines if needed
    void showText(const char* heading, const char* text, uint16_t bg) {
      int w = M5.Display.width(), h = M5.Display.height();
      M5.Display.fillScreen(bg);
      M5.Display.setTextDatum(textdatum_t::middle_center);
      M5.Display.setTextColor(TFT_WHITE);

      M5.Display.setFont(&fonts::DejaVu18);
      M5.Display.setTextSize(1);
      M5.Display.setTextColor(EYE_RGB(220, 220, 220));
      M5.Display.drawString(heading, w / 2, h / 6);
      M5.Display.setTextColor(TFT_WHITE);

      M5.Display.setFont(&fonts::DejaVu40);
      M5.Display.setTextSize(1);
      int full = M5.Display.textWidth(text);
      if (full <= w - 16 || strchr(text, ' ') == nullptr) {
        M5.Display.setTextSize(min(2.0f, (w - 16) / (float)full));
        M5.Display.drawString(text, w / 2, h / 2);
        return;
      }
      // Split at the space closest to the middle
      int len = strlen(text), best = -1;
      for (int k = 0; k < len; k++) {
        if (text[k] == ' ' && (best < 0 || abs(k - len / 2) < abs(best - len / 2))) {
          best = k;
        }
      }
      char l1[48], l2[48];
      snprintf(l1, sizeof(l1), "%.*s", best, text);
      snprintf(l2, sizeof(l2), "%s", text + best + 1);
      int widest = max((int)M5.Display.textWidth(l1), (int)M5.Display.textWidth(l2));
      M5.Display.setTextSize(min(2.0f, (w - 16) / (float)widest));
      int lh = M5.Display.fontHeight();
      M5.Display.drawString(l1, w / 2, h / 2 - lh / 2);
      M5.Display.drawString(l2, w / 2, h / 2 + lh / 2);
    }

    // ---- Animal drawing helpers (k values are in units of u) ----
    int X(float k) const { return cx + (int)(k * u); }
    int Y(float k) const { return cy + (int)(k * u); }
    int S(float k) const { return max(1, (int)(k * u)); }

    void circ(float x, float y, float r, uint16_t c) { M5.Display.fillCircle(X(x), Y(y), S(r), c); }
    void ell(float x, float y, float rx, float ry, uint16_t c) { M5.Display.fillEllipse(X(x), Y(y), S(rx), S(ry), c); }
    void tri(float x0, float y0, float x1, float y1, float x2, float y2, uint16_t c) {
      M5.Display.fillTriangle(X(x0), Y(y0), X(x1), Y(y1), X(x2), Y(y2), c);
    }
    void line(float x0, float y0, float x1, float y1, float r, uint16_t c) {
      M5.Display.drawWideLine(X(x0), Y(y0), X(x1), Y(y1), max(1.0f, r * u), c);
    }
    // Pair of eyes: white with black pupil and a highlight
    void eyes(float y, float dx, float r) {
      for (int s = -1; s <= 1; s += 2) {
        circ(s * dx, y, r, TFT_WHITE);
        circ(s * dx, y, r * 0.55f, TFT_BLACK);
        circ(s * dx - r * 0.2f, y - r * 0.2f, r * 0.18f, TFT_WHITE);
      }
    }
    // Simple black dot eyes with a highlight
    void dots(float y, float dx, float r) {
      for (int s = -1; s <= 1; s += 2) {
        circ(s * dx, y, r, TFT_BLACK);
        circ(s * dx - r * 0.3f, y - r * 0.3f, r * 0.3f, TFT_WHITE);
      }
    }
    void smile(float x, float y, float r, uint16_t c = TFT_BLACK) {
      M5.Display.fillArc(X(x), Y(y), S(r), max(0, S(r) - S(0.25f)), 20, 160, c);
    }
    void whiskers(float y, float x0, float x1) {
      for (int s = -1; s <= 1; s += 2) {
        line(s * x0, y, s * x1, y - 0.5f, 0.06f, TFT_BLACK);
        line(s * x0, y + 0.3f, s * x1, y + 0.8f, 0.06f, TFT_BLACK);
      }
    }

    void drawAnimal(int i) {
      int w = M5.Display.width(), h = M5.Display.height();
      u = max(2, min(w, h) / 12);
      cx = w / 2;
      cy = h / 2 - u;

      static const uint16_t bgs[ANIMAL_COUNT] = {
        EYE_RGB(120, 200, 255), EYE_RGB(150, 220, 120), EYE_RGB(180, 230, 140), EYE_RGB(110, 190, 90),
        EYE_RGB(130, 200, 255), EYE_RGB(170, 230, 255), EYE_RGB(30, 90, 180), EYE_RGB(20, 20, 70),
        EYE_RGB(200, 235, 255), EYE_RGB(255, 230, 180), EYE_RGB(150, 210, 110), EYE_RGB(120, 180, 90),
        EYE_RGB(140, 200, 100), EYE_RGB(240, 210, 120), EYE_RGB(60, 140, 70), EYE_RGB(40, 90, 60),
        EYE_RGB(150, 220, 255), EYE_RGB(80, 160, 230), EYE_RGB(240, 200, 140), EYE_RGB(140, 210, 255),
        EYE_RGB(60, 150, 60), EYE_RGB(130, 180, 120), EYE_RGB(120, 200, 100), EYE_RGB(90, 180, 80),
        EYE_RGB(150, 210, 255), EYE_RGB(255, 220, 240), EYE_RGB(90, 180, 220), EYE_RGB(230, 210, 150),
        EYE_RGB(20, 70, 140), EYE_RGB(40, 100, 180)
      };
      M5.Display.fillScreen(bgs[i]);

      const uint16_t BLK = TFT_BLACK, WHT = TFT_WHITE;
      const uint16_t PINK = EYE_RGB(255, 140, 170), ORNG = EYE_RGB(255, 150, 30);
      switch (i) {
        case 0: {  // Cat
          uint16_t c = EYE_RGB(255, 160, 60);
          tri(-4, -1, -3.5f, -5.5f, -0.8f, -3.5f, c);
          tri(4, -1, 3.5f, -5.5f, 0.8f, -3.5f, c);
          tri(-3.4f, -2, -3.2f, -4.5f, -1.6f, -3.3f, PINK);
          tri(3.4f, -2, 3.2f, -4.5f, 1.6f, -3.3f, PINK);
          circ(0, 0, 4, c);
          for (int s = -1; s <= 1; s += 2) {
            ell(s * 1.6f, -0.8f, 0.9f, 1.1f, EYE_RGB(120, 220, 60));
            ell(s * 1.6f, -0.8f, 0.25f, 0.95f, BLK);
          }
          tri(-0.5f, 0.6f, 0.5f, 0.6f, 0, 1.2f, PINK);
          whiskers(1.3f, 1.2f, 4.8f);
          smile(-0.4f, 1.2f, 0.45f);
          smile(0.4f, 1.2f, 0.45f);
          break;
        }
        case 1: {  // Dog
          uint16_t dark = EYE_RGB(120, 70, 30);
          ell(-3.6f, 0.5f, 1.3f, 2.8f, dark);
          ell(3.6f, 0.5f, 1.3f, 2.8f, dark);
          circ(0, 0, 3.8f, EYE_RGB(210, 160, 100));
          ell(0, 1.6f, 2.1f, 1.6f, EYE_RGB(245, 225, 190));
          dots(-0.8f, 1.5f, 0.6f);
          circ(0, 3.1f, 0.7f, PINK);
          ell(0, 0.9f, 0.8f, 0.55f, BLK);
          smile(-0.6f, 1.6f, 0.6f);
          smile(0.6f, 1.6f, 0.6f);
          break;
        }
        case 2: {  // Pig
          uint16_t dark = EYE_RGB(240, 120, 150);
          tri(-3.6f, -2, -3.2f, -5, -1, -3.5f, dark);
          tri(3.6f, -2, 3.2f, -5, 1, -3.5f, dark);
          circ(0, 0, 4, EYE_RGB(255, 180, 200));
          ell(0, 1, 1.9f, 1.3f, dark);
          ell(-0.6f, 1, 0.3f, 0.5f, EYE_RGB(150, 50, 80));
          ell(0.6f, 1, 0.3f, 0.5f, EYE_RGB(150, 50, 80));
          dots(-1.2f, 1.6f, 0.5f);
          break;
        }
        case 3: {  // Cow
          tri(-2.5f, -3, -3.5f, -5.5f, -1.2f, -3.6f, EYE_RGB(240, 230, 200));
          tri(2.5f, -3, 3.5f, -5.5f, 1.2f, -3.6f, EYE_RGB(240, 230, 200));
          ell(-4, -1.5f, 1.3f, 0.6f, BLK);
          ell(4, -1.5f, 1.3f, 0.6f, BLK);
          ell(0, 0, 3.4f, 4, WHT);
          circ(-1.8f, -2, 1.1f, BLK);
          circ(2, 0.2f, 0.9f, BLK);
          ell(0, 2.6f, 2.6f, 1.5f, PINK);
          circ(-0.9f, 2.6f, 0.35f, EYE_RGB(150, 60, 80));
          circ(0.9f, 2.6f, 0.35f, EYE_RGB(150, 60, 80));
          dots(-0.9f, 1.4f, 0.55f);
          break;
        }
        case 4: {  // Sheep
          for (int k = 0; k < 10; k++) {
            float a = k * 2 * M_PI / 10;
            circ(cosf(a) * 3.4f, sinf(a) * 3.4f, 1.6f, WHT);
          }
          circ(0, 0, 3.2f, WHT);
          ell(-2.6f, -0.2f, 1.1f, 0.5f, EYE_RGB(40, 40, 40));
          ell(2.6f, -0.2f, 1.1f, 0.5f, EYE_RGB(40, 40, 40));
          ell(0, 0.6f, 1.9f, 2.5f, EYE_RGB(40, 40, 40));
          eyes(0, 0.8f, 0.55f);
          smile(0, 1.8f, 0.5f, WHT);
          break;
        }
        case 5: {  // Frog
          uint16_t c = EYE_RGB(70, 190, 70);
          circ(-2.2f, -2, 1.6f, c);
          circ(2.2f, -2, 1.6f, c);
          ell(0, 0.8f, 4.6f, 3, c);
          eyes(-2, 2.2f, 1.1f);
          smile(0, 0.2f, 3, EYE_RGB(20, 90, 20));
          circ(-3, 1.5f, 0.6f, PINK);
          circ(3, 1.5f, 0.6f, PINK);
          break;
        }
        case 6: {  // Fish
          tri(2.6f, 0, 5.2f, -2.6f, 5.2f, 2.6f, ORNG);
          ell(-0.5f, 0, 3.6f, 2.4f, ORNG);
          tri(-0.5f, -2.2f, 1.5f, -3.6f, 1.5f, -1.8f, EYE_RGB(255, 110, 0));
          for (int k = 0; k < 3; k++) {
            M5.Display.fillArc(X(0.2f + k * 0.9f), Y(0), S(1.6f), S(1.4f), -60, 60, EYE_RGB(255, 200, 120));
          }
          circ(-2.4f, -0.5f, 0.75f, WHT);
          circ(-2.5f, -0.5f, 0.4f, BLK);
          smile(-3.3f, 0.2f, 0.5f);
          M5.Display.drawCircle(X(-4.5f), Y(-2.5f), S(0.4f), WHT);
          M5.Display.drawCircle(X(-4.9f), Y(-3.7f), S(0.6f), WHT);
          M5.Display.drawCircle(X(-4.2f), Y(-5), S(0.3f), WHT);
          break;
        }
        case 7: {  // Owl
          uint16_t c = EYE_RGB(140, 90, 50);
          tri(-3, -2.5f, -3.2f, -5.2f, -1.2f, -3.6f, c);
          tri(3, -2.5f, 3.2f, -5.2f, 1.2f, -3.6f, c);
          ell(0, 0.5f, 3.6f, 4.4f, c);
          ell(0, 2, 2.4f, 2.6f, EYE_RGB(220, 190, 140));
          for (int s = -1; s <= 1; s += 2) {
            circ(s * 1.5f, -1, 1.4f, WHT);
            circ(s * 1.5f, -1, 1.0f, EYE_RGB(255, 200, 0));
            circ(s * 1.5f, -1, 0.55f, BLK);
          }
          tri(-0.5f, -0.2f, 0.5f, -0.2f, 0, 0.9f, ORNG);
          break;
        }
        case 8: {  // Penguin
          ell(0, 0.5f, 3.3f, 4.6f, EYE_RGB(30, 30, 40));
          ell(0, 1.3f, 2.4f, 3.6f, WHT);
          ell(-3.3f, 1.2f, 0.6f, 2.2f, EYE_RGB(30, 30, 40));
          ell(3.3f, 1.2f, 0.6f, 2.2f, EYE_RGB(30, 30, 40));
          dots(-1.5f, 0.9f, 0.45f);
          tri(-0.6f, -0.7f, 0.6f, -0.7f, 0, 0.3f, ORNG);
          ell(-1.2f, 5, 1, 0.4f, ORNG);
          ell(1.2f, 5, 1, 0.4f, ORNG);
          break;
        }
        case 9: {  // Mouse
          uint16_t c = EYE_RGB(170, 170, 180);
          circ(-3, -2.8f, 1.9f, c);
          circ(3, -2.8f, 1.9f, c);
          circ(-3, -2.8f, 1.2f, PINK);
          circ(3, -2.8f, 1.2f, PINK);
          circ(0, 0.5f, 3.2f, c);
          dots(-0.2f, 1.2f, 0.5f);
          circ(0, 1.4f, 0.5f, PINK);
          whiskers(1.5f, 0.8f, 4.2f);
          break;
        }
        case 10: {  // Rabbit
          ell(-1.3f, -4, 0.9f, 2.8f, WHT);
          ell(1.3f, -4, 0.9f, 2.8f, WHT);
          ell(-1.3f, -4, 0.45f, 2.2f, PINK);
          ell(1.3f, -4, 0.45f, 2.2f, PINK);
          circ(0, 0.8f, 3, WHT);
          dots(0.2f, 1.2f, 0.45f);
          tri(-0.4f, 1.2f, 0.4f, 1.2f, 0, 1.7f, PINK);
          M5.Display.fillRect(X(-0.4f), Y(2), S(0.8f), S(0.7f), WHT);
          M5.Display.drawRect(X(-0.4f), Y(2), S(0.8f), S(0.7f), EYE_RGB(150, 150, 150));
          circ(-2, 1.8f, 0.5f, PINK);
          circ(2, 1.8f, 0.5f, PINK);
          break;
        }
        case 11: {  // Bear
          uint16_t c = EYE_RGB(140, 90, 50), tanC = EYE_RGB(215, 170, 120);
          circ(-3, -3, 1.3f, c);
          circ(3, -3, 1.3f, c);
          circ(-3, -3, 0.7f, tanC);
          circ(3, -3, 0.7f, tanC);
          circ(0, 0, 3.8f, c);
          ell(0, 1.3f, 1.8f, 1.3f, tanC);
          ell(0, 0.7f, 0.7f, 0.5f, BLK);
          dots(-1, 1.5f, 0.5f);
          smile(0, 1.4f, 0.7f);
          break;
        }
        case 12: {  // Panda
          circ(-3, -3, 1.3f, BLK);
          circ(3, -3, 1.3f, BLK);
          circ(0, 0, 3.8f, WHT);
          ell(-1.5f, -0.5f, 1, 1.3f, BLK);
          ell(1.5f, -0.5f, 1, 1.3f, BLK);
          circ(-1.4f, -0.7f, 0.4f, WHT);
          circ(1.4f, -0.7f, 0.4f, WHT);
          circ(-1.4f, -0.7f, 0.2f, BLK);
          circ(1.4f, -0.7f, 0.2f, BLK);
          ell(0, 1, 0.6f, 0.4f, BLK);
          smile(0, 1.5f, 0.7f);
          break;
        }
        case 13: {  // Lion
          uint16_t mane = EYE_RGB(200, 110, 30);
          for (int k = 0; k < 12; k++) {
            float a = k * 2 * M_PI / 12;
            circ(cosf(a) * 3.8f, sinf(a) * 3.8f, 1.5f, mane);
          }
          circ(0, 0, 4, mane);
          circ(-2.3f, -2.3f, 0.8f, EYE_RGB(240, 190, 90));
          circ(2.3f, -2.3f, 0.8f, EYE_RGB(240, 190, 90));
          circ(0, 0, 3.1f, EYE_RGB(240, 190, 90));
          ell(0, 1.4f, 1.6f, 1.1f, EYE_RGB(255, 230, 170));
          dots(-0.8f, 1.3f, 0.5f);
          tri(-0.6f, 0.6f, 0.6f, 0.6f, 0, 1.3f, EYE_RGB(110, 60, 30));
          smile(-0.4f, 1.4f, 0.45f);
          smile(0.4f, 1.4f, 0.45f);
          break;
        }
        case 14: {  // Tiger
          uint16_t c = EYE_RGB(255, 140, 20);
          circ(-3, -3, 1.2f, c);
          circ(3, -3, 1.2f, c);
          circ(-3, -3, 0.6f, WHT);
          circ(3, -3, 0.6f, WHT);
          circ(0, 0, 4, c);
          ell(-1.3f, 1.6f, 1.7f, 1.3f, WHT);
          ell(1.3f, 1.6f, 1.7f, 1.3f, WHT);
          tri(-0.5f, -4, 0.5f, -4, 0, -2.4f, BLK);
          tri(-1.6f, -3.7f, -0.9f, -3.8f, -1.2f, -2.6f, BLK);
          tri(1.6f, -3.7f, 0.9f, -3.8f, 1.2f, -2.6f, BLK);
          for (int s = -1; s <= 1; s += 2) {
            tri(s * 4, -1, s * 4, -0.2f, s * 2.8f, -0.6f, BLK);
            tri(s * 3.9f, 0.6f, s * 3.8f, 1.4f, s * 2.8f, 1, BLK);
          }
          eyes(-0.9f, 1.5f, 0.6f);
          tri(-0.5f, 0.8f, 0.5f, 0.8f, 0, 1.4f, PINK);
          whiskers(1.8f, 1.5f, 4.5f);
          break;
        }
        case 15: {  // Fox
          uint16_t c = EYE_RGB(240, 110, 20);
          tri(-4.4f, -1.5f, -3.6f, -5.5f, -1.2f, -2.8f, c);
          tri(4.4f, -1.5f, 3.6f, -5.5f, 1.2f, -2.8f, c);
          tri(-3.6f, -2, -3.3f, -4.5f, -2, -3, BLK);
          tri(3.6f, -2, 3.3f, -4.5f, 2, -3, BLK);
          tri(-4.6f, -2.2f, 4.6f, -2.2f, 0, 3.6f, c);
          tri(-4.6f, -2.2f, -0.4f, 0.5f, 0, 3.6f, WHT);
          tri(4.6f, -2.2f, 0.4f, 0.5f, 0, 3.6f, WHT);
          circ(0, 3.2f, 0.55f, BLK);
          dots(-0.9f, 1.6f, 0.5f);
          break;
        }
        case 16: {  // Chick
          uint16_t c = EYE_RGB(255, 220, 40);
          circ(-0.6f, -3.6f, 0.6f, ORNG);
          circ(0.3f, -3.9f, 0.6f, ORNG);
          circ(0, 0, 3.6f, c);
          ell(-3.4f, 1, 0.8f, 1.6f, EYE_RGB(240, 190, 20));
          ell(3.4f, 1, 0.8f, 1.6f, EYE_RGB(240, 190, 20));
          dots(-0.8f, 1.3f, 0.55f);
          tri(-0.7f, 0.2f, 0.7f, 0.2f, 0, 1.3f, ORNG);
          circ(-2.2f, 0.9f, 0.5f, PINK);
          circ(2.2f, 0.9f, 0.5f, PINK);
          tri(-1.2f, 3.5f, -0.6f, 4.5f, -1.8f, 4.5f, ORNG);
          tri(1.2f, 3.5f, 0.6f, 4.5f, 1.8f, 4.5f, ORNG);
          break;
        }
        case 17: {  // Duck
          circ(0, -0.5f, 3.6f, EYE_RGB(255, 225, 50));
          dots(-1.6f, 1.3f, 0.55f);
          ell(0, 1.1f, 2.3f, 0.9f, ORNG);
          M5.Display.drawFastHLine(X(-2), Y(1.1f), S(4), EYE_RGB(200, 100, 0));
          circ(-2.4f, 0, 0.5f, PINK);
          circ(2.4f, 0, 0.5f, PINK);
          M5.Display.fillRect(X(-6), Y(3.5f), S(12), S(3), EYE_RGB(60, 130, 210));
          break;
        }
        case 18: {  // Elephant
          uint16_t c = EYE_RGB(150, 150, 165);
          ell(-3.6f, 0, 2.3f, 3, c);
          ell(3.6f, 0, 2.3f, 3, c);
          ell(-3.6f, 0, 1.5f, 2.2f, PINK);
          ell(3.6f, 0, 1.5f, 2.2f, PINK);
          circ(0, -0.5f, 3, c);
          line(0, 0.5f, 0, 3.8f, 0.8f, c);
          line(0, 3.8f, 1.2f, 4.4f, 0.7f, c);
          dots(-1.1f, 1.2f, 0.45f);
          tri(-1.4f, 1.4f, -0.9f, 1.4f, -2.2f, 2.6f, WHT);
          tri(1.4f, 1.4f, 0.9f, 1.4f, 2.2f, 2.6f, WHT);
          break;
        }
        case 19: {  // Giraffe
          uint16_t c = EYE_RGB(250, 200, 70), sp = EYE_RGB(180, 110, 40);
          M5.Display.fillRect(X(-1), Y(1), S(2), S(6), c);
          circ(-0.4f, 3, 0.5f, sp);
          circ(0.5f, 4.5f, 0.5f, sp);
          line(-0.8f, -2.5f, -1.2f, -4.6f, 0.15f, sp);
          line(0.8f, -2.5f, 1.2f, -4.6f, 0.15f, sp);
          circ(-1.2f, -4.6f, 0.45f, sp);
          circ(1.2f, -4.6f, 0.45f, sp);
          ell(-2.4f, -2.2f, 1, 0.45f, c);
          ell(2.4f, -2.2f, 1, 0.45f, c);
          ell(0, -1, 2.2f, 2.8f, c);
          circ(-1.2f, -2.6f, 0.4f, sp);
          circ(1.4f, -2, 0.35f, sp);
          ell(0, 0.8f, 1.6f, 1.1f, EYE_RGB(255, 225, 150));
          circ(-0.5f, 0.6f, 0.2f, sp);
          circ(0.5f, 0.6f, 0.2f, sp);
          dots(-1.2f, 1, 0.45f);
          break;
        }
        case 20: {  // Monkey
          uint16_t c = EYE_RGB(130, 80, 40), tanC = EYE_RGB(235, 195, 150);
          circ(-3.6f, 0, 1.2f, c);
          circ(3.6f, 0, 1.2f, c);
          circ(-3.6f, 0, 0.7f, tanC);
          circ(3.6f, 0, 0.7f, tanC);
          circ(0, 0, 3.5f, c);
          circ(-1, -0.6f, 1.4f, tanC);
          circ(1, -0.6f, 1.4f, tanC);
          ell(0, 1.2f, 2.3f, 1.6f, tanC);
          dots(-0.6f, 1, 0.5f);
          circ(-0.3f, 0.7f, 0.15f, c);
          circ(0.3f, 0.7f, 0.15f, c);
          smile(0, 1.2f, 1);
          break;
        }
        case 21: {  // Koala
          uint16_t c = EYE_RGB(160, 160, 170);
          circ(-3.3f, -2.4f, 1.9f, c);
          circ(3.3f, -2.4f, 1.9f, c);
          circ(-3.3f, -2.4f, 1.1f, WHT);
          circ(3.3f, -2.4f, 1.1f, WHT);
          circ(0, 0, 3.5f, c);
          dots(-0.8f, 1.6f, 0.45f);
          ell(0, 0.7f, 0.9f, 1.3f, EYE_RGB(40, 40, 50));
          break;
        }
        case 22: {  // Snail
          uint16_t body = EYE_RGB(230, 200, 150);
          ell(0, 2.5f, 4.8f, 1, body);
          circ(-3.8f, 1.2f, 1.2f, body);
          line(-4.1f, 0.4f, -4.6f, -1.6f, 0.12f, body);
          line(-3.5f, 0.4f, -3.4f, -1.6f, 0.12f, body);
          circ(-4.6f, -1.6f, 0.4f, BLK);
          circ(-3.4f, -1.6f, 0.4f, BLK);
          smile(-3.9f, 1.3f, 0.5f);
          circ(0.6f, 0, 2.8f, EYE_RGB(170, 90, 40));
          for (int k = 1; k <= 3; k++) {
            M5.Display.drawCircle(X(0.6f + k * 0.2f), Y(k * 0.15f), S(2.8f - k * 0.7f), EYE_RGB(240, 180, 110));
          }
          break;
        }
        case 23: {  // Ladybug
          circ(0, -3, 1.8f, BLK);
          circ(-0.7f, -3.4f, 0.35f, WHT);
          circ(0.7f, -3.4f, 0.35f, WHT);
          line(-0.6f, -4.4f, -1.4f, -5.5f, 0.1f, BLK);
          line(0.6f, -4.4f, 1.4f, -5.5f, 0.1f, BLK);
          circ(0, 0.6f, 3.8f, EYE_RGB(230, 20, 20));
          line(0, -3.1f, 0, 4.3f, 0.1f, BLK);
          circ(-1.8f, -0.8f, 0.7f, BLK);
          circ(1.8f, -0.8f, 0.7f, BLK);
          circ(-2, 1.8f, 0.6f, BLK);
          circ(2, 1.8f, 0.6f, BLK);
          circ(-0.9f, 3.2f, 0.5f, BLK);
          circ(0.9f, 3.2f, 0.5f, BLK);
          break;
        }
        case 24: {  // Bee
          ell(-0.8f, -2.4f, 1.3f, 1.8f, EYE_RGB(220, 240, 255));
          ell(1, -2.4f, 1.3f, 1.8f, EYE_RGB(220, 240, 255));
          tri(3.6f, 0.2f, 3.6f, 1.2f, 4.8f, 0.7f, BLK);
          ell(0.2f, 0.7f, 3.6f, 2.5f, EYE_RGB(255, 210, 0));
          ell(-0.6f, 0.7f, 0.4f, 2.4f, BLK);
          ell(1.1f, 0.7f, 0.4f, 2.3f, BLK);
          ell(2.6f, 0.7f, 0.35f, 1.6f, BLK);
          circ(-3.6f, 0.4f, 1.5f, BLK);
          circ(-4, 0, 0.45f, WHT);
          circ(-4, 0, 0.2f, BLK);
          line(-3.6f, -1, -4.4f, -2.8f, 0.1f, BLK);
          line(-3.2f, -1, -2.8f, -2.8f, 0.1f, BLK);
          break;
        }
        case 25: {  // Butterfly
          ell(-2.2f, -1.6f, 2.2f, 2.4f, EYE_RGB(150, 60, 220));
          ell(2.2f, -1.6f, 2.2f, 2.4f, EYE_RGB(150, 60, 220));
          ell(-1.8f, 2, 1.6f, 1.8f, EYE_RGB(255, 120, 200));
          ell(1.8f, 2, 1.6f, 1.8f, EYE_RGB(255, 120, 200));
          circ(-2.4f, -1.8f, 0.9f, EYE_RGB(255, 220, 60));
          circ(2.4f, -1.8f, 0.9f, EYE_RGB(255, 220, 60));
          circ(-1.8f, 2.2f, 0.6f, WHT);
          circ(1.8f, 2.2f, 0.6f, WHT);
          ell(0, 0.3f, 0.45f, 3.4f, BLK);
          line(0, -3, -1.2f, -5, 0.1f, BLK);
          line(0, -3, 1.2f, -5, 0.1f, BLK);
          circ(-1.2f, -5, 0.3f, BLK);
          circ(1.2f, -5, 0.3f, BLK);
          break;
        }
        case 26: {  // Turtle
          uint16_t skin = EYE_RGB(130, 200, 90);
          ell(-2.5f, 2.2f, 0.9f, 0.7f, skin);
          ell(2.5f, 2.2f, 0.9f, 0.7f, skin);
          tri(-3.6f, 0.8f, -4.8f, 1.6f, -3.4f, 1.6f, skin);
          circ(3.9f, -0.4f, 1.2f, skin);
          circ(4.3f, -0.8f, 0.3f, BLK);
          smile(4.1f, -0.3f, 0.5f);
          ell(0, 0.5f, 3.6f, 2.4f, EYE_RGB(40, 120, 50));
          circ(0, 0.2f, 0.9f, EYE_RGB(90, 170, 70));
          circ(-1.8f, 0.6f, 0.7f, EYE_RGB(90, 170, 70));
          circ(1.8f, 0.6f, 0.7f, EYE_RGB(90, 170, 70));
          circ(-0.9f, -1.1f, 0.6f, EYE_RGB(90, 170, 70));
          circ(0.9f, -1.1f, 0.6f, EYE_RGB(90, 170, 70));
          break;
        }
        case 27: {  // Snake
          uint16_t c = EYE_RGB(60, 170, 60);
          line(-4, 4, -1.5f, 3, 0.8f, c);
          line(-1.5f, 3, 1.5f, 2.6f, 0.8f, c);
          line(1.5f, 2.6f, 2.5f, 1, 0.8f, c);
          line(2.5f, 1, 0.5f, -0.3f, 0.8f, c);
          line(0.5f, -0.3f, -1.5f, -1, 0.8f, c);
          line(-1.5f, -1, -0.8f, -2.6f, 0.8f, c);
          ell(0.5f, -3, 1.8f, 1.3f, c);
          line(2.2f, -2.8f, 3.4f, -2.8f, 0.08f, EYE_RGB(230, 0, 0));
          tri(3.3f, -2.8f, 3.8f, -3.2f, 3.8f, -2.4f, EYE_RGB(230, 0, 0));
          eyes(-3.4f, 0.7f, 0.45f);
          break;
        }
        case 28: {  // Octopus
          uint16_t c = EYE_RGB(190, 80, 200);
          for (int k = 0; k < 6; k++) {
            float x0 = -2.5f + k * 1.0f;
            float x1 = -4 + k * 1.6f;
            line(x0, 1, x1, 3.6f, 0.45f, c);
            circ(x1 + ((k < 3) ? -0.3f : 0.3f), 4, 0.45f, c);
          }
          ell(0, -1, 3.2f, 3, c);
          eyes(-1.2f, 1.3f, 0.8f);
          smile(0, 0.2f, 0.9f);
          circ(-2.3f, 0.3f, 0.45f, PINK);
          circ(2.3f, 0.3f, 0.45f, PINK);
          break;
        }
        case 29: {  // Whale
          uint16_t c = EYE_RGB(60, 120, 220);
          line(0, -2, -0.6f, -4.5f, 0.15f, EYE_RGB(180, 230, 255));
          line(0, -2, 0.6f, -4.5f, 0.15f, EYE_RGB(180, 230, 255));
          line(0, -2, 0, -4.8f, 0.15f, EYE_RGB(180, 230, 255));
          tri(3, 0.8f, 5.2f, -1.4f, 5.2f, 2.6f, c);
          ell(-0.5f, 0.8f, 4, 2.6f, c);
          ell(-0.8f, 2, 3.2f, 1.2f, EYE_RGB(190, 220, 255));
          circ(-2.6f, 0.2f, 0.45f, BLK);
          smile(-3, 0.9f, 0.7f);
          break;
        }
      }

      // Name label at the bottom
      M5.Display.setFont(&fonts::DejaVu40);
      M5.Display.setTextSize(1);
      float sz = min(1.0f, (w - 16) / (float)M5.Display.textWidth(animalNames[i]));
      M5.Display.setTextSize(sz);
      M5.Display.setTextDatum(textdatum_t::bottom_center);
      M5.Display.setTextColor(TFT_WHITE);
      int lx = w / 2, ly = h - 8;
      // Dark outline so the label reads on any background
      M5.Display.setTextColor(TFT_BLACK);
      M5.Display.drawString(animalNames[i], lx - 2, ly);
      M5.Display.drawString(animalNames[i], lx + 2, ly);
      M5.Display.drawString(animalNames[i], lx, ly - 2);
      M5.Display.drawString(animalNames[i], lx, ly + 2);
      M5.Display.setTextColor(TFT_WHITE);
      M5.Display.drawString(animalNames[i], lx, ly);
    }
};

#endif