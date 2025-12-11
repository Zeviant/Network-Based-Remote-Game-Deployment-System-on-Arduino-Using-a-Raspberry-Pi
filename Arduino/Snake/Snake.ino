#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

const int VRx = A0;
const int VRy = A1;
const int BTN = 4;
const int BUZZER = 3;
const int G_LED = 10;

Adafruit_SH1106G display = Adafruit_SH1106G(128, 64, &Wire);

const int SCREEN_W = 128;
const int SCREEN_H = 64;

const int CELL = 8;
const int COLS = 16;  // 128 / 8
const int ROWS = 8;   // 64 / 8

// FULL playable area (0..15, 0..7)
const int8_t PLAY_MIN_X = 0;
const int8_t PLAY_MIN_Y = 0;
const int8_t PLAY_MAX_X = COLS - 1;  // 15
const int8_t PLAY_MAX_Y = ROWS - 1;  // 7

const uint16_t MAX_LENGTH = COLS * ROWS;

int8_t snakeX[MAX_LENGTH];
int8_t snakeY[MAX_LENGTH];
uint16_t snakeLength = 3;

int score = 0;

int8_t foodX, foodY;

enum Direction {UP, DOWN, LEFT, RIGHT};
Direction dir = RIGHT;

bool gameOver = false;
bool inStartScreen = true;

unsigned long lastMove = 0;
const int MOVE_INTERVAL = 240;

// ------------------------------------------------------------

void placeFood() {
  while (true) {
    foodX = random(PLAY_MIN_X, PLAY_MAX_X + 1);
    foodY = random(PLAY_MIN_Y, PLAY_MAX_Y + 1);

    bool conflict = false;
    for (int i = 0; i < snakeLength; i++) {
      if (snakeX[i] == foodX && snakeY[i] == foodY) {
        conflict = true;
        break;
      }
    }
    if (!conflict) return;
  }
}

void resetGame() {
  snakeLength = 3;
  score = 0;

  snakeX[0] = 5; snakeY[0] = 4;
  snakeX[1] = 4; snakeY[1] = 4;
  snakeX[2] = 3; snakeY[2] = 4;

  dir = RIGHT;
  gameOver = false;

  analogWrite(G_LED, 0); // Turn LED off

  placeFood();
}

void drawCell(int8_t gx, int8_t gy, bool filled) {
  int px = gx * CELL;
  int py = gy * CELL;

  if (filled)
    display.fillRect(px, py, CELL, CELL, SH110X_WHITE);
  else
    display.drawRect(px, py, CELL, CELL, SH110X_WHITE);
}

void handleJoystick() {
  int x = analogRead(VRx);
  int y = analogRead(VRy);

  if (x < 100 && dir != LEFT) dir = RIGHT;
  else if (x > 900 && dir != RIGHT) dir = LEFT;
  else if (y < 100 && dir != UP) dir = DOWN;
  else if (y > 900 && dir != DOWN) dir = UP;
}

bool isCollision(int8_t x, int8_t y) {
  for (int i = 1; i < snakeLength; i++) {
    if (snakeX[i] == x && snakeY[i] == y) return true;
  }
  return false;
}

void moveSnake() {
  for (int i = snakeLength - 1; i > 0; i--) {
    snakeX[i] = snakeX[i - 1];
    snakeY[i] = snakeY[i - 1];
  }

  if (dir == UP) snakeY[0]--;
  else if (dir == DOWN) snakeY[0]++;
  else if (dir == LEFT) snakeX[0]--;
  else if (dir == RIGHT) snakeX[0]++;

  if (snakeX[0] < 0 || snakeX[0] > PLAY_MAX_X ||
      snakeY[0] < 0 || snakeY[0] > PLAY_MAX_Y) {

    display.display();
    delay(1500);

    gameOver = true;
    analogWrite(G_LED, 0);
    return;
  }

  // Self collision
  if (isCollision(snakeX[0], snakeY[0])) {
    display.display();
    delay(1500);

    gameOver = true;
    analogWrite(G_LED, 0);
    return;
  }

  // Food eaten
  if (snakeX[0] == foodX && snakeY[0] == foodY) {
    if (snakeLength < MAX_LENGTH) {
      snakeX[snakeLength] = snakeX[snakeLength - 1];
      snakeY[snakeLength] = snakeY[snakeLength - 1];
      snakeLength++;
    }

    score++;

    // LED fade from 0 → 255 when score goes 0 → 100
    int ledLevel = score;
    if (ledLevel > 100) ledLevel = 100;
    analogWrite(G_LED, map(ledLevel, 0, 100, 0, 255));

    tone(BUZZER, 600, 40);
    placeFood();
  }
}

// ------------------------------------------------------------

void setup() {
  pinMode(BTN, INPUT_PULLUP);
  pinMode(BUZZER, OUTPUT);
  pinMode(G_LED, OUTPUT);

  analogWrite(G_LED, 0);

  display.begin(0x3C, true);
  display.setTextColor(SH110X_WHITE);

  randomSeed(analogRead(0));

  resetGame();
}

void loop() {

  // ---------- START SCREEN ----------
  if (inStartScreen) {
    display.clearDisplay();

    display.setTextSize(2);
    display.setCursor(36, 15);
    display.print(F("SNAKE"));

    display.setTextSize(1);
    display.setCursor(0, 45);
    display.print(F("Press Button to Start"));

    display.display();

    if (digitalRead(BTN) == LOW) {
      delay(200);
      inStartScreen = false;
      resetGame();
    }
    return;
  }

  // ---------- GAME OVER RESTART ----------
  if (digitalRead(BTN) == LOW && gameOver) {
    delay(200);
    resetGame();
  }

  handleJoystick();

  if (!gameOver && millis() - lastMove > MOVE_INTERVAL) {
    lastMove = millis();
    moveSnake();
  }

  display.clearDisplay();

  if (gameOver) {
    display.setTextSize(2);
    display.setCursor(8, 20);
    display.print(F("GAME OVER"));

    display.setTextSize(1);
    display.setCursor(26, 48);
    display.print(F("Press Button"));

    display.display();
    return;
  }

  // Visual border only (non-lethal)
  display.drawRect(0, 0, SCREEN_W, SCREEN_H, SH110X_WHITE);

  // Score
  display.setTextSize(1);
  display.setCursor(2, 2);
  display.print(F("Score: "));
  display.print(score);

  // Food
  drawCell(foodX, foodY, true);

  // Snake
  for (int i = 0; i < snakeLength; i++) {
    drawCell(snakeX[i], snakeY[i], i == 0);
  }

  display.display();
}
