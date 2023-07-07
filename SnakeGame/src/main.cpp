#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "bitmap.h"
#include <EEPROM.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

const int kGameWidth = 64, kGameHeight = 32, kMaxLength = 464, kStartLength = 6;

struct Position {
  int x, y;
  bool operator==(const Position& rhs) const {
    return x == rhs.x && y == rhs.y;
  }
  Position& operator+=(const Position& rhs) {
    x += rhs.x;
    y += rhs.y;
    return *this;
  }
};

void draw_square(Position pos, int color = WHITE) {
  display.fillRect(pos.x * 2, pos.y * 2, 2, 2, color);
}

bool test_position(Position pos) {
  return display.getPixel(pos.x * 2, pos.y * 2);
}

const Position kDirPos[4] = {
  {0, -1}, {1, 0}, {0, 1}, {-1, 0}
};

struct Player {
  Player() {
    reset();
  }
  Position pos;
  unsigned char tail[kMaxLength];
  unsigned char direction;
  int size, moved;
  void reset() {
    pos = {32, 16};
    direction = 1;
    size = kStartLength;
    memset(tail, 0, sizeof(tail));
    moved = 0;
  }
  void turn_left() {
    direction = (direction + 3) % 4;
  }

  void turn_right() {
    direction = (direction + 1) % 4;
  }

  void update() {
    for (int i = kMaxLength - 1; i > 0; --i) {
      tail[i] = tail[i] << 2 | ((tail[i - 1] >> 6) & 3);
    }
    tail[0] = tail[0] << 2 | ((direction + 2) % 4);
    pos += kDirPos[direction];
    if (moved < size) {
      moved++;
    }
  }
  void render() const {
    draw_square(pos);
    if (moved < size) {
      return;
    }
    Position tailpos = pos;
    for (int i = 0; i < size; ++i) {
      tailpos += kDirPos[tail[(i >> 2)] >> ((i & 3) * 2) & 3];
      draw_square(tailpos);
    }
    draw_square(tailpos, BLACK);
  }
};

Player player;

struct Item {
  Position pos;
  void spawn() {
    pos.x = random(1, 63);
    pos.y = random(1, 31);
  }
  void render() const {
    draw_square(pos);
  }
} item;

void wait_for_input() {
  while (!Serial.available()) {
    // Wait for serial input
  }
}

void push_to_start() {
  display.setCursor(26, 57);
  display.print("Press any button to start");
}

void flash_screen() {
  display.invertDisplay(true);
  delay(100);
  display.invertDisplay(false);
  delay(200);
}

void play_intro() {
  display.clearDisplay();
  display.drawBitmap(0, 0, StartGame, 128, 64, 1);
  push_to_start();
  display.display();
  wait_for_input();
  flash_screen();
}

void play_outro() {
  flash_screen();
  display.clearDisplay();
  display.drawBitmap(0, 0, WinGame, 128, 64, 1);
  push_to_start();
  display.display();
  wait_for_input();
  flash_screen();
}

void play_gameover() {
  flash_screen();
  display.clearDisplay();
  display.drawBitmap(0, 0, GameOver, 128, 64, 1);
  int score = player.size - kStartLength;
  display.setCursor(50, 57);
  display.print("Score: ");
  display.print(score);
  int hiscore;
  EEPROM.get(0, hiscore);
  if (score > hiscore) {
    EEPROM.put(0, score);
    hiscore = score;
    display.setCursor(26, 57);
    display.print("New High Score!");
  }
  display.setCursor(26, 57);
  display.print("High Score: ");
  display.print(hiscore);
  push_to_start();
  display.display();
  wait_for_input();
  flash_screen();
}

void reset_game() {
  display.clearDisplay();
  for (char x = 0; x < kGameWidth; ++x) {
    draw_square({x, 0});
    draw_square({x, 31});
  }
  for (char y = 0; y < kGameHeight; ++y) {
    draw_square({0, y});
    draw_square({63, y});
  }
  player.reset();
  item.spawn();
}

void update_game() {
  player.update();

  if (player.pos == item.pos) {
    player.size++;
    item.spawn();
  } else if (test_position(player.pos)) {
    play_gameover();
    reset_game();
  }
}

void render_game() {
  player.render();
  item.render();
  display.display();
}

void setup() {
  Serial.begin(9600);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for (;;) {
    }
  }
  display.setTextColor(WHITE);
  play_intro();
  reset_game();
}

void loop() {
  if (Serial.available() > 0) {
    char command = Serial.read();
    if (command == 'A') {
      player.turn_left();
    } else if (command == 'D') {
      player.turn_right();
    } else if (command == 'W') {
      player.turn_up();
    } else if (command == 'S') {
      player.turn_down();
    }
  }
  update_game();
  render_game();
  delay(100);
}
