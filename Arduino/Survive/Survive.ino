#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

// ==== DISPLAY SETUP ====
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire);

// ==== INPUT PINS ====
const int VRx = A0;     // Joystick X
const int VRy = A1;     // Joystick Y
const int SW  = 2;      // Joystick switch
const int BTN = 4;      // Action button

// ==== OUTPUT PINS ====
const int BUZZER = 3;
const int R_LED  = 9;
const int G_LED  = 10;
const int B_LED  = 11;

// ==== THRESHOLDS FOR JOYSTICK ====
const int upperThresh = 900;
const int lowerThresh = 100;

// ==== GAME STATES ====
enum GameState {
  START,
  PLAY,
  END
};

GameState gameState = START;

// player variables
uint8_t playerY = 32;
uint8_t playerX = 32;
bool invincible = false;
unsigned long invincibleEndTime = 0;
bool dodgeReady = true;
unsigned long dodgeCooldownEnd = 0;

const unsigned long INVINCE_MS = 1000;   // invincibility duration when dodge pressed
const unsigned long COOLDOWN_MS = 2000; // dodge cooldown

uint8_t finalScore = 0;

// mortar
#define MAX_MORTARS 20
struct Mortar{
  uint8_t x;
  uint8_t y;
  unsigned long spawnTime;
  unsigned long explodeTime; // moment when explosion activates
  bool exploded;            // has blown up (and been handled)
  uint8_t radius;           // radius of the mortar target
};

Mortar mortars[MAX_MORTARS];
uint8_t mortarCount = 0;

// Timing and difficulty helpers
unsigned long startTime = 0;
unsigned long lastSpawnTime = 0;
unsigned long currentTime = 0;


// to see better
void updateInputs();
void updateLED();
void drawPlayer();
void spawnMortar(uint8_t x, uint8_t y, unsigned long delayMs, uint8_t radius, bool tracking);
void updateMortars();
void drawMortars();
void checkCollisions();
bool rectCircleCollision(int cx, int cy, int cr, int rx, int ry, int rw, int rh);

// update input
void updateInputs() {
  int joystickY = analogRead(VRy);
  int joystickX = analogRead(VRx);


  // Move up
  if (joystickY < lowerThresh) {
    playerY += 6;
  }
  // Move down
  else if (joystickY > upperThresh) {
    playerY -= 6;
  }

  // Move left
  if (joystickX < lowerThresh) {
    playerX += 6;
  }
  // Move right
  else if (joystickX > upperThresh) {
    playerX -= 6;
  }

  playerX = constrain(playerX, 10, SCREEN_WIDTH - 10);
  playerY = constrain(playerY, 10, SCREEN_HEIGHT - 10);

  // Button to start or "action"
  if (digitalRead(BTN) == LOW) {
    if (gameState == START) {
      gameState = PLAY;
      startTime = millis();
      mortarCount = 0;
    }
    else if(gameState == PLAY){
      if(dodgeReady){
        invincible = true;
        invincibleEndTime = millis() + INVINCE_MS;
        dodgeReady = false;
        dodgeCooldownEnd = millis() + COOLDOWN_MS;

        tone(BUZZER, 1200, 40);
      }
    }
  }

  if(invincible && millis() >= invincibleEndTime){
    invincible = false;
  }
  if(!dodgeReady && millis() >= dodgeCooldownEnd){
    dodgeReady = true;
  }

  // Joystick switch to reset
  if (digitalRead(SW) == LOW && gameState == END) {
    gameState = START;
  }
}

//led depends on dodge/invincible
void updateLED() {
  if(invincible){
    digitalWrite(R_LED, LOW);
    digitalWrite(G_LED, LOW);
    digitalWrite(B_LED, HIGH);
  }
  else if(dodgeReady){
    digitalWrite(R_LED, LOW);
    digitalWrite(G_LED, HIGH);
    digitalWrite(B_LED, LOW);
  }
  else{
    digitalWrite(R_LED, HIGH);
    digitalWrite(G_LED, LOW);
    digitalWrite(B_LED, LOW);
  }
  
}

//drawing player. the drawing also depends on the dodge/invincible condition
void drawPlayer() {
  if(invincible){
    display.fillCircle(playerX - 4, playerY, 1, SH110X_WHITE);
    display.fillCircle(playerX + 4, playerY, 1, SH110X_WHITE);
    display.fillCircle(playerX, playerY - 4, 1, SH110X_WHITE);
    display.fillCircle(playerX, playerY + 4, 1, SH110X_WHITE);
    display.fillCircle(playerX, playerY, 3, SH110X_WHITE);
  }
  else if(dodgeReady){
      display.fillCircle(playerX, playerY, 2, SH110X_WHITE);
      display.fillCircle(playerX - 3, playerY, 2, SH110X_WHITE);
      display.fillCircle(playerX + 3, playerY, 2, SH110X_WHITE);
      display.fillCircle(playerX, playerY - 3, 2, SH110X_WHITE);
      display.fillCircle(playerX, playerY + 3, 2, SH110X_WHITE);
  }
  else{
    display.fillCircle(playerX - 3, playerY, 2, SH110X_WHITE);
    display.fillCircle(playerX + 3, playerY, 2, SH110X_WHITE);
    display.fillCircle(playerX, playerY - 3, 2, SH110X_WHITE);
    display.fillCircle(playerX, playerY + 3, 2, SH110X_WHITE);
  }
}

//mortars, the only enemy of the game
void spawnMortar(uint8_t x, uint8_t y, unsigned long delayMs, uint8_t radius, bool tracking){
  if(mortarCount >= MAX_MORTARS) return;
  Mortar &m = mortars[mortarCount++];
  if(tracking){
    m.x = constrain(playerX + random(-12, 13), 8, SCREEN_WIDTH - 8);
    m.y = constrain(playerY + random(-12, 13), 8, SCREEN_WIDTH - 8);
  }
  else{
    m.x = x;
    m.y = y;
  }
 
  m.spawnTime = millis();
  m.explodeTime = m.spawnTime + delayMs + random(0, 300);
  m.exploded = false;
  m.radius = radius + random(0, 3);
}

void updateMortars(){
  for(int i = 0; i < mortarCount; i++){
    Mortar &m = mortars[i];
    if(m.exploded && millis() - m.explodeTime > 300){
      mortarCount--;
      mortars[i] = mortars[mortarCount];
      i--;
      continue;
    }
    if (!m.exploded && millis() >= m.explodeTime){
      m.exploded = true;
      //buzzer here later
    }
  }
}

void drawMortars(){
  for(int i = 0; i < mortarCount; i++){
    Mortar &m = mortars[i];
    if(!m.exploded){
      display.drawCircle(m.x, m.y, m.radius, SH110X_WHITE);
    }
    else{
      display.fillCircle(m.x, m.y, m.radius, SH110X_WHITE);
    }
  }
}

// collisions
bool rectCircleCollision(int cx, int cy, int cr, int rx, int ry, int rw, int rh) {
  // closest point on rect to circle center
  int closestX = max(rx, min(cx, rx + rw));
  int closestY = max(ry, min(cy, ry + rh));
  int dx = cx - closestX;
  int dy = cy - closestY;
  return (dx * dx + dy * dy) <= (cr * cr);
}

void checkCollisions() {
  if (gameState != PLAY) return;
  if (invincible) return; // ignore collisions while invincible

  // player rectangle
  int pW = 8;
  int pH = 8;
  int pX = playerX - pW/2;
  int pY = playerY - pH/2;

  // mortar collisions (only when exploded)
  for (int i = 0; i < mortarCount; i++) {
    Mortar &m = mortars[i];
    if (m.exploded) {
      if (rectCircleCollision(m.x, m.y, m.radius, pX, pY, pW, pH)) {
        // hit by mortar explosion
        gameState = END;
        finalScore = (currentTime - startTime) / 1000;
        return;
      }
    }
  }
}

//setup and loop
void setup() {
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
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);


  startTime = millis();
  lastSpawnTime = startTime;
}

void loop() {
  currentTime = millis();

  updateInputs();
  updateMortars();
  checkCollisions();
  updateLED();

  display.clearDisplay();

  if (gameState == START) {
    display.setCursor(0, 0);
    display.setTextSize(2);
    display.println("Survive!!!");
    display.setTextSize(1);
    display.setCursor(0, 32);
    display.println("Press BTN to start");
    display.setTextColor(SH110X_WHITE);

  }

  else if (gameState == PLAY) {
    if (currentTime - lastSpawnTime > (unsigned long)max(50UL, 500UL - ((currentTime - startTime)*2 / 1000))) {
      spawnMortar(random(16, SCREEN_WIDTH - 16), random(10, SCREEN_HEIGHT - 10), 1250UL, 6, false);
      spawnMortar(0, 0, 1250UL, 6, true);
      
      //implementation for patterns, has issues probably with the amount of projectiles and game balance
     /* 
      uint8_t pattern = random(0, 5);

    switch (pattern) {
      case 0: // single intelligent mortar
        spawnMortar(0, 0, 1250 + random(0, 1000), random(4,12), true);
        break;

      case 1: // triple barrage
        for (uint8_t k=0; k<3; k++)
          spawnMortar(0, 0,
                      1250 + random(1000,1300), random(4,10), true);
        break;

      case 2: // horizontal line
      uint8_t y_ran = random(10, SCREEN_HEIGHT-10);
        for (uint8_t k=20; k<SCREEN_WIDTH-20; k+=20)
          spawnMortar(k, y_ran,
                      1250 + random(0,800), 5, false);
        break;

      case 3: // vertical line barrage
        uint8_t x_ran = random(20, SCREEN_WIDTH-20);
        for (uint8_t k=10; k<SCREEN_HEIGHT-10; k+=16)
          spawnMortar(x_ran, k,
                      1250 + random(0,700), 5, false);
        break;

      case 4: // big one intelligent
        spawnMortar(playerX, playerY, 1250 + 1200, 12, true);
        break;
    }
    */
      lastSpawnTime = currentTime;
    }
    drawMortars();
    drawPlayer();
    display.setCursor(0, 0);
    display.println("SCORE: ");
    display.print((currentTime - startTime) / 1000);
  }

  else if (gameState == END) {
    display.setCursor(0, 32);
    display.println("GAME OVER");
    display.println("Press SW to reset");
    display.print("Final Score: ");
    display.print(finalScore);
  }

  display.display();
}
