#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <stdint.h>

// UNUSED PINS: VRx, B_LED. Can still just uncomment the code though.

// -- Arduino logic --

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire);

const int VRx = A0;
const int VRy = A1;

const int SW = 2;
const int BTN = 4;

const int BUZZER = 3;

bool lastButtonState = HIGH;

const int R_LED = 9;
const int G_LED = 10;
const int B_LED = 11;

const int upperThresh = 800;
const int lowerThresh = 200;
bool joystickUpPressed = false;
bool joystickDownPressed = false;

uint8_t rState = LOW, gState = LOW, bState = LOW;

// -- Game logic --

// GAME STATES
enum GAMESTATE
{
  START,
  ONGOING,
  END,
  TRUE_END
};

GAMESTATE gameState = START;

// Progression Flags
bool lvl1 = true;
bool lvl2 = true;
bool lvl3 = true;
bool lvl4 = true;
bool lvl5_intermediate = true;
bool lvl5 = true;
bool lvl6 = true;
bool lvl7 = true;
bool lvl8 = true;
bool lvl9 = true;
bool lvl10_intermediate = true;
bool lvl10 = true;
bool finalSequenceStartup = false;
bool finalSequence = false;
bool ending = false;
unsigned long endingTime = 4294967295UL;
bool cinematicStarter = true;
#define CINEMATIC_START_TIME 3000UL

// LINES
#define UI_LINE 10
const int LINE_1_POS = 24;
const int LINE_2_POS = 40;
const int LINE_3_POS = 56;
int lines[3] = {LINE_1_POS, LINE_2_POS, LINE_3_POS};
uint8_t currentLine = 1;

// PLAYER
#define PLAYER_SIZE 5
uint8_t charPosX = 16;               
uint8_t charPosY = LINE_2_POS;
uint8_t speed = 3;
int8_t health = 5;               
int score = 0;

// ENEMY
#define MAX_ENEMIES 14          
#define ENEMY_SIZE 8
uint8_t enemySpeed = 2;
uint8_t enemyCount = 0;
int enemySpawnRate = 1000;

struct Enemy
{
  int x;
  uint8_t y;
};

Enemy enemies[MAX_ENEMIES];

// BULLETS
#define MAX_BULLETS 8           
#define BULLET_SIZE_X 6
#define BULLET_SIZE_Y 3
uint8_t bulletSpeed = 5;
uint8_t bulletCount = 0;

struct Bullet
{
  int x;
  uint8_t y;
};

Bullet bullets[MAX_BULLETS];

// BACKGROUND
#define MAX_STARS 14         
#define STAR_SIZE 1
uint8_t starCount = 0;
uint8_t starSpeed = 1;
int starSpawnRate = 400;

struct Star
{
  int x;
  uint8_t y;
};

Star stars[MAX_STARS];

// OTHER
#define TEXT_BLINK_RATE 800
bool textVisible = true;
bool loopingAllowed = true;

// -- Arduino-related functions -- 

// Reset function (Not sure if best practice)
void(* resetFunc) (void) = 0;

void updateLED()
{
  rState = LOW;
  gState = LOW;
  bState = LOW;

  if (health == 5 || health == 4)
  {
    gState = HIGH;
  }
  else if (health == 3 || health == 2)
  {
    rState = HIGH;
    gState = HIGH;
  }
  else if (health == 1)
  {
    rState = HIGH;
  }
  else
  {
    rState = LOW;
    gState = LOW;
    bState = LOW;
  }

  digitalWrite(R_LED, rState);
  digitalWrite(G_LED, gState);
  digitalWrite(B_LED, bState);
}

void spawnBullet();
void spawnStar();

void updateButton()
{
  bool currentButtonState = digitalRead(BTN);
  bool currentSwitchState = digitalRead(SW);

  if (currentButtonState == LOW && lastButtonState == HIGH)
  {
    if (gameState == ONGOING)
    {
      spawnBullet();
    }
    else if (gameState == START)
    {
      gameState = ONGOING;
    }
  }
  lastButtonState = currentButtonState;

  if (currentSwitchState == LOW && gameState == END)
  {
    resetFunc();
  }
  else if (currentSwitchState == LOW && gameState == ONGOING)
  {
    score += 10;
    health = 5;
  }
}

// -- Game Functions -- 

void updateDifficulty()
{
  if (score >= 10 && lvl1)
  {
    enemySpawnRate = 800;
    lvl1 = false;
  }
  else if (score >= 30 && lvl2)
  {
    enemySpawnRate = 600;
    lvl2 = false;
  }
  else if (score >= 60 && lvl3)
  {
    enemySpawnRate = 400;
    lvl3 = false;
  }
  else if (score >= 150 && lvl4)
  {
    enemySpawnRate = 375;
    lvl4 = false;
  }
  else if (score >= 340 && lvl5_intermediate)
  {
    enemySpawnRate = 700;
    lvl5_intermediate = false;
  }
  else if (score >= 350 && lvl5)
  {
    enemySpeed = 3;
    starSpeed = 2;
    lvl5 = false;
  }
  else if (score >= 370 && lvl6)
  {
    enemySpawnRate = 550;
    lvl6 = false;
  }
  else if (score >= 400 && lvl7)
  {
    enemySpawnRate = 450;
    lvl7 = false;
  }
  else if (score >= 500 && lvl8)
  {
    enemySpawnRate = 360;
    lvl8 = false;
  }
  else if (score >= 700 && lvl9)
  {
    enemySpawnRate = 330;
    lvl9 = false;
  }
  else if (score >= 880 && lvl10_intermediate)
  {
    finalSequenceStartup = true;
    lvl10_intermediate = false;
  }
  else if (score >= 900 && lvl10)
  {
    finalSequence = true;
    charPosY = lines[1];
    starSpeed = 4;
    enemySpawnRate = 175;
    lvl10 = false;
  }
  else if (score >= 1000)
  {
    ending = true;

    if (cinematicStarter)
    {
      endingTime = millis();
      cinematicStarter = false;
    }
  }
}

void spawnEnemy()
{
  if (enemyCount < MAX_ENEMIES)
  {
    int line = random(0, 3);
    if (finalSequenceStartup)
    {
      line = 1;
    }

    enemies[enemyCount].x = SCREEN_WIDTH + ENEMY_SIZE;

    if (line == 0)
    {
      enemies[enemyCount].y = LINE_1_POS - ENEMY_SIZE / 2;
    }
    else if (line == 1)
    {
      enemies[enemyCount].y = LINE_2_POS - ENEMY_SIZE / 2;
    }
    else
    {
      enemies[enemyCount].y = LINE_3_POS - ENEMY_SIZE / 2;
    }

    enemyCount++;
  }
}

void spawnBullet()
{
  if (bulletCount < MAX_BULLETS)
  {
    bullets[bulletCount].x = charPosX + PLAYER_SIZE + 1;
    bullets[bulletCount].y = charPosY - 1;
    bulletCount++;

    tone(BUZZER, 700, 40);
  }
}

void starSpawner()
{
  static unsigned long lastStarSpawnTime = 0;
  if (millis() - lastStarSpawnTime > (unsigned long)starSpawnRate)
  {
    spawnStar();
    lastStarSpawnTime = millis();
  }
}

void enemySpawner()
{
  if (!ending)
  {
    static unsigned long lastEnemySpawnTime = 0;
    if (millis() - lastEnemySpawnTime > (unsigned long)enemySpawnRate)
    {
      spawnEnemy();
      lastEnemySpawnTime = millis();
    }
  }
}

bool playerCollisionChecker(int xPlayer, int yPlayer, int xEnemy, int yEnemy)
{
  // Player collision box
  int playerLeft = xPlayer - PLAYER_SIZE;
  int playerRight = xPlayer + PLAYER_SIZE;
  int playerTop = yPlayer - PLAYER_SIZE;
  int playerBottom = yPlayer + PLAYER_SIZE;

  // Enemy collision box
  int enemyLeft = xEnemy;
  int enemyRight = xEnemy + ENEMY_SIZE;
  int enemyTop = yEnemy;
  int enemyBottom = yEnemy + ENEMY_SIZE;

  return !(playerRight < enemyLeft || playerLeft > enemyRight || playerBottom < enemyTop || playerTop > enemyBottom);
}

bool bulletCollisionChecker(int xBullet, int yBullet, int xEnemy, int yEnemy)
{
  // Bullet collision box 
  int bulletLeft = xBullet;
  int bulletRight = xBullet + BULLET_SIZE_X;
  int bulletTop = yBullet;
  int bulletBottom = yBullet + BULLET_SIZE_Y;

  // Enemy collision box
  int enemyLeft = xEnemy;
  int enemyRight = xEnemy + ENEMY_SIZE;
  int enemyTop = yEnemy;
  int enemyBottom = yEnemy + ENEMY_SIZE;

  return !(bulletRight < enemyLeft || bulletLeft > enemyRight || bulletBottom < enemyTop || bulletTop > enemyBottom);
}

void drawLines()
{
  for (int x = 0; x < SCREEN_WIDTH; x += 2)
  {
    if (!finalSequence)
    {
      display.drawPixel(x, LINE_1_POS, SH110X_WHITE);
    }

    display.drawPixel(x, LINE_2_POS, SH110X_WHITE);

    if (!finalSequence)
    {
      display.drawPixel(x, LINE_3_POS, SH110X_WHITE);
    }
  }
  display.drawLine(0, UI_LINE, SCREEN_WIDTH, UI_LINE, SH110X_WHITE);

  if (!finalSequence)
  {
    display.drawLine(SCREEN_WIDTH / 2, LINE_1_POS + 3, SCREEN_WIDTH / 2, LINE_1_POS - 3, SH110X_WHITE);
  }

  display.drawLine(SCREEN_WIDTH / 2, LINE_2_POS + 3, SCREEN_WIDTH / 2, LINE_2_POS - 3, SH110X_WHITE);

  if (!finalSequence)
  {
    display.drawLine(SCREEN_WIDTH / 2, LINE_3_POS + 3, SCREEN_WIDTH / 2, LINE_3_POS - 3, SH110X_WHITE);
  }
}

void updatePlayer()
{
  if (!finalSequence)
  {
    int joystickY = analogRead(VRy);

    if (joystickY > upperThresh)
    {
      if (currentLine > 0 && !joystickUpPressed)
      {
        currentLine--;
        joystickUpPressed = true;
      }
      else if (currentLine == 0 && !joystickUpPressed && loopingAllowed)
      {
        currentLine = 2;
        joystickUpPressed = true;
      }
    }
    else if (joystickY < lowerThresh)
    {
      if (currentLine < 2 && !joystickDownPressed)
      {
        currentLine++;
        joystickDownPressed = true;
      }
      else if (currentLine == 2 && !joystickDownPressed && loopingAllowed)
      {
        currentLine = 0;
        joystickDownPressed = true;
      }
    }
    else
    {
      joystickUpPressed = false;
      joystickDownPressed = false;
    }

    charPosY = lines[currentLine];
  }
  else if (ending && (millis() - endingTime > CINEMATIC_START_TIME))
  {
    charPosX = charPosX + speed;
    starSpeed = 5;
    if (charPosX > 220)
    {
      gameState = TRUE_END;
    }
  }
}

void drawPlayer(int x, int y)
{
  int x1 = x + PLAYER_SIZE;     // POINT AT TIP
  int y1 = y;

  int x2 = x - PLAYER_SIZE;     // BOTTOM POINT
  int y2 = y + PLAYER_SIZE;

  int x3 = x - PLAYER_SIZE;     // TOP POINT
  int y3 = y - PLAYER_SIZE;

  display.fillTriangle(x1, y1, x2, y2, x3, y3, SH110X_WHITE);
}

void updateEnemies()
{
  for (uint8_t i = 0; i < enemyCount; i++)
  {
    enemies[i].x = enemies[i].x - enemySpeed;

    // Off-screen removal
    if (enemies[i].x + ENEMY_SIZE < 0)
    {
      enemyCount--;
      enemies[i] = enemies[enemyCount];
      i--;
      health--;
    }
  }
}

void drawEnemies()
{
  for (uint8_t i = 0; i < enemyCount; i++)
  {
    display.fillRect(enemies[i].x, enemies[i].y, ENEMY_SIZE, ENEMY_SIZE, SH110X_WHITE);

    // Enemy-Player collision
    if (playerCollisionChecker(charPosX, charPosY, enemies[i].x, enemies[i].y))
    {
      enemyCount--;
      enemies[i] = enemies[enemyCount];
      i--;
      health--;
    }
  }
}

void updateBullets()
{
  for (uint8_t i = 0; i < bulletCount; i++)
  {
    bullets[i].x = bullets[i].x + bulletSpeed;

    if (bullets[i].x - BULLET_SIZE_X > SCREEN_WIDTH / 2)
    {
      bulletCount--;
      bullets[i] = bullets[bulletCount];
      i--;

      if (score > 0 && !ending)
      {
        score--;
      }
    }
  }
}

void drawBullets()
{
  for (uint8_t i = 0; i < bulletCount; i++)
  {
    display.fillRect(bullets[i].x, bullets[i].y, BULLET_SIZE_X, BULLET_SIZE_Y, SH110X_WHITE);

    for (uint8_t j = 0; j < enemyCount; j++)
    {
      // Bullet-Enemy collision
      if (bulletCollisionChecker(bullets[i].x, bullets[i].y, enemies[j].x, enemies[j].y))
      {
        score++;
        updateDifficulty();

        // remove enemy
        enemyCount--;
        enemies[j] = enemies[enemyCount];
        j--;

        // remove bullet
        bulletCount--;
        bullets[i] = bullets[bulletCount];
        i--;

        break;
      }
    }
  }
}

void spawnStar()
{
  if (starCount < MAX_STARS)
  {
    // Generate random star position (23-25, 39-41, 54-56 banned to keep the lines tidy)
    int randPos;
    do {
      randPos = random(16, 64);
    } while ((randPos >= 23 && randPos <= 25) || (randPos >= 39 && randPos <= 41) || (randPos >= 54 && randPos <= 56));

    stars[starCount].x = SCREEN_WIDTH + STAR_SIZE;
    stars[starCount].y = randPos;
    starCount++;

    if (gameState == START || gameState == ONGOING)
    {
      if (enemySpeed == 3 && !finalSequence)
      {
        starSpawnRate = random(300, 600);
      }
      else if (enemySpeed == 3 && finalSequence)
      {
        starSpawnRate = random(100, 200);
      }
      else
      {
        starSpawnRate = random(400, 800);
      }
    }
    else if (gameState == END)
    {
      starSpawnRate = random(200, 400);
      starSpeed = 2;
    }
  }
}

void updateStars()
{
  for (uint8_t i = 0; i < starCount; i++)
  {
    stars[i].x = stars[i].x - starSpeed;

    // Off-screen removal
    if (stars[i].x + STAR_SIZE < 0)
    {
      starCount--;
      stars[i] = stars[starCount];
      i--;
    }
  }
}

void drawStars()
{
  for (uint8_t i = 0; i < starCount; i++)
  {
    display.drawPixel(stars[i].x, stars[i].y, SH110X_WHITE);
  }
}

void deathEffect() {
  for (int i = 255; i >= 0; i--)
  {
    analogWrite(R_LED, i);
    delay(4); // ~1s
  }
  analogWrite(R_LED, 0);
}

// --------------------------------------------------
// setup / loop
// --------------------------------------------------

void setup()
{
pinMode(SW, INPUT_PULLUP);
  pinMode(BTN, INPUT_PULLUP);
  pinMode(BUZZER, OUTPUT);

  pinMode(R_LED, OUTPUT);
  pinMode(G_LED, OUTPUT);
  pinMode(B_LED, OUTPUT);

  display.begin(0x3C, true);
  delay(200);
  display.clearDisplay();
  display.display();

  gameState = START;
}

void loop()
{
  if (gameState == START)
  {
    updateStars();
    updateButton();
    starSpawner();

    static unsigned long lastBlinkTime = 0;
    if (millis() - lastBlinkTime > TEXT_BLINK_RATE)
    {
      lastBlinkTime = millis();
      textVisible = !textVisible;
    }

    display.clearDisplay();
    drawStars();
    display.setCursor(4, 0);
    display.setTextSize(2);
    display.setTextColor(SH110X_WHITE);
    display.println(F("ASTRAL ATK"));
    display.setCursor(8, 32);
    display.setTextSize(1);
    if (textVisible)
    {
      display.println(F("PRESS BTN TO START"));
    }
    display.display();
  }

  if (gameState == ONGOING)
  {
    // Updates
    updatePlayer();
    updateEnemies();
    updateLED();
    updateButton();
    updateBullets();
    updateStars();

    // Spawners
    starSpawner();
    enemySpawner();

    // Renders
    display.clearDisplay();
    drawStars();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    display.print(F("SCORE: "));
    if (score < 1000)
    {
      display.println(score);
    }
    else
    {
      display.println(F("MAX"));
    }
    drawLines();
    drawEnemies();
    drawBullets();
    drawPlayer(charPosX, charPosY);
    display.display();

    if (health <= 0)
    {
      deathEffect();
      delay(1000);
      gameState = END;
    }
  }

  if (gameState == END)
  {
    updateStars();
    updateButton();
    starSpawner();

    static unsigned long lastBlinkTime2 = 0;
    if (millis() - lastBlinkTime2 > TEXT_BLINK_RATE)
    {
      lastBlinkTime2 = millis();
      textVisible = !textVisible;
    }

    display.clearDisplay();
    drawStars();
    display.setCursor(6, 0);
    display.setTextSize(2);
    display.setTextColor(SH110X_WHITE);
    display.println(F("GAME OVER!"));
    display.setCursor(1, 32);
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    if (textVisible)
    {
      display.println(F("PRESS JOYSTICK SWITCH"));
    }
    display.display();
  }

  if (gameState == TRUE_END)
  {
    static unsigned long lastBlinkTime3 = 0;
    if (millis() - lastBlinkTime3 > TEXT_BLINK_RATE)
    {
      lastBlinkTime3 = millis();
      textVisible = !textVisible;
    }

    display.clearDisplay();
    display.setCursor(20, 24);
    display.setTextSize(2);
    display.setTextColor(SH110X_WHITE);
    display.println(F("YOU WON!"));
    if (textVisible)
    {
      display.drawRect(5, 20, 118, 24, SH110X_WHITE);
    }
    display.display();
  }
  delay(1);
}
