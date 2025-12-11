#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <stdint.h>

// -- Arduino stuff logic --

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire);

const int VRx = A1;
const int VRy = A0;

const int SW = 2;
const int BTN = 4;

const int BUZZER = 3;

const int R_LED = 9;
const int G_LED = 10;
const int B_LED = 11;

const int upperThresh = 900;
const int lowerThresh = 100;
bool joystickUpPressed = false;
bool joystickDownPressed = false;
bool joystickLeftPressed = false;
bool joystickRightPressed = false;
bool buttonPressed = false;
bool switchPressed = false;

bool lastButtonState = HIGH;
bool lastSwitchState = HIGH;

uint8_t rState = LOW, gState = LOW, bState = LOW;
int x;

// -- Game Logic --

// GAME STATES
enum GAMESTATE
{
  START,
  PLAYERS,
  ONGOING,
  END,
  END_MULTI
};

GAMESTATE gameState = START;

// INSTRUCTIONS
enum INSTRUCTIONS
{
  NONE,   //0
  UP,     //1
  DOWN,   //2
  RIGHT,  //3
  LEFT,   //4
  SMASH,  //5
  BOPIT,  //6
  LOSE,   //7
  NEXT    //8
};

INSTRUCTIONS instruction = NONE;
int currentInstr = 0;
int instrCtr = 0;

// GAME VARIABLES
#define MIN_FOR_BOP_IT 3
#define WAIT_FOR_CHANGE 500
unsigned long gameSpeed = 3050;
int playerCount = 1;
int playerSet = 1;
int currentPlayer = 1;
bool playerState[4] = {true, true, true, true};
bool success = true;

// OTHER
#define TEXT_BLINK_RATE 800
bool textVisible = true;

// Reset function (Not sure if best practice)
void(* resetFunc) (void) = 0;

// -- Functions --
// MISC
void rgb_handler() {
  analogWrite(R_LED, LOW);
  analogWrite(G_LED, LOW);
  analogWrite(B_LED, LOW);

  if (playerCount == 1) {
    x = (gameSpeed - 550)/10;

    analogWrite(R_LED, 250 - x);
    analogWrite(G_LED, x);
  }
  else {
    switch(currentPlayer) {
      case 1:
        analogWrite(G_LED, 255); break;
      case 2:
        analogWrite(B_LED, 255); break;
      case 3:
        analogWrite(R_LED, 255); analogWrite(G_LED, 255); break;
      case 4:
        analogWrite(R_LED, 255); break;
    }
  }
}

void message() {
  display.clearDisplay();
  display.setCursor(12, 24);
  display.setTextSize(2);
  display.setTextColor(SH110X_WHITE);
  switch (instruction) {
    case UP:  display.println(F("FLY UP!")); break;
    case DOWN:  display.println(F("LET DOWN!")); break;
    case RIGHT:  display.println(F("GO RIGHT!")); break;
    case LEFT:  display.println(F("TRY LEFT!")); break;
    case SMASH:  display.println(F("SMASH IT!")); break;
    case BOPIT:  display.println(F("BOP IT!")); break;
    case LOSE:  display.println(F("YOU LOSE!")); break;
    case NEXT:  display.println(F("THE NEXT~")); break;
    default: display.println(F("...")); break;
  }
  display.display();
}

void blink_screen() {
  display.clearDisplay();
  display.display();
  delay(50);
  message();
  display.clearDisplay();
  display.display();
  delay(50);

  if((instruction == LOSE || instruction == BOPIT) && playerCount > 1) {
    instruction = NEXT;
    message(); 

    do {
      currentPlayer = (currentPlayer % playerSet) + 1;
    } while (playerState[currentPlayer] == false);

    delay(950);
  }
}

// GAME
void tasker_solo() {
  static unsigned long instructionTimer = 0;

  if (success == true || millis() - instructionTimer > gameSpeed) {
    if (success) {
      currentInstr = random(1, 7);
      
      instruction = static_cast<INSTRUCTIONS>(currentInstr);

      instructionTimer = millis();
      success = false;
      if (gameSpeed > 500) gameSpeed -= 50;

      message();
    }
    else {
      display.clearDisplay();
      display.display();
      for (int i = 0; i <= 255; i++) {
        analogWrite(R_LED, max(0, 255 - x - i));
        analogWrite(G_LED, max(0, x - i));
        delay(4); // ~1s
      }
      gameState = END;
    }
  }
}

void tasker_multi() {
  static unsigned long instructionTimer = 0;

  if (success == true || millis() - instructionTimer > gameSpeed) {
    if (success) {
      currentInstr = random(1, 7);
      
      if (currentInstr == 6) {
        if (instrCtr <= 3) {
          while (currentInstr == 6) {
            currentInstr = random(1, 7);
          }
        }
        else {
          instrCtr = -1;
        }
      }
      instrCtr++;

      instruction = static_cast<INSTRUCTIONS>(currentInstr);

      instructionTimer = millis();
      success = false;
      if (gameSpeed > 500) gameSpeed -= 25;

      message();
    }
    else {
      playerState[currentPlayer] = false;
      playerCount--;
      
      instruction = LOSE;

      message();
      blink_screen();

      if (playerCount == 1) {
        for (int i = 255; i >= 0; i--) {
          switch(currentPlayer) {
            case 1:
              analogWrite(G_LED, i); break;
            case 2:
              analogWrite(B_LED, i); break;
            case 3:
              analogWrite(R_LED, i); analogWrite(G_LED, i); break;
            case 4:
              analogWrite(R_LED, i); break;
          }
          delay(4); // ~1s
        }

        do {
          currentPlayer = (currentPlayer % playerSet) + 1;
        } while (playerState[currentPlayer] == false);

        gameState = END_MULTI;
      }

      instructionTimer = millis();
      success = true;
    }
  }
}

void controller() {
  bool currentButtonState = digitalRead(BTN);
  bool currentSwitchState = digitalRead(SW);

  int joystickX = analogRead(VRx);
  int joystickY = analogRead(VRy);

  if (joystickY < lowerThresh && !joystickUpPressed) {
    joystickRightPressed = true;
  }
  else if (joystickY > upperThresh && !joystickDownPressed) {
    joystickLeftPressed = true;
  }
  else if (joystickX < lowerThresh && !joystickRightPressed) {
    joystickDownPressed = true;
  }
  else if (joystickX > upperThresh && !joystickLeftPressed) {
    joystickUpPressed = true;
  }
  else {
    joystickUpPressed = false;
    joystickDownPressed = false;
    joystickRightPressed = false;
    joystickLeftPressed = false;
  }

  //Buttons Handlers
  if (currentButtonState == LOW && lastButtonState == HIGH) {
    buttonPressed = true;
  }
  else {
    buttonPressed = false;
  }
  lastButtonState = currentButtonState;

  if (currentSwitchState == LOW && lastSwitchState == HIGH) {
    switchPressed = true;
  }
  else {
    switchPressed = false;
  }
  lastSwitchState = currentSwitchState;
}

void decider() {
  bool wasSuccess = success;

  switch (instruction) {
    case UP: 
      if (joystickUpPressed) success = true; 
      break;

    case DOWN: 
      if (joystickDownPressed) success = true; 
      break;

    case RIGHT: 
      if (joystickRightPressed) success = true; 
      break;

    case LEFT: 
      if (joystickLeftPressed) success = true; 
      break;

    case SMASH: 
      if (switchPressed) success = true; 
      break;

    case BOPIT: 
      if (buttonPressed) success = true; 
      break;

    default: 
      break;
  }

  if (success && !wasSuccess) {
    tone(BUZZER, 900, 40); 
  }

  if (success) blink_screen();
}

// UTILITY
void changeScreen()
{
  controller();

  if (buttonPressed && gameState == PLAYERS)
  {
    for (int i = 0; i <= 255; i++) {
        analogWrite(G_LED, i);
        delay(4); // ~1s
    }
    gameState = ONGOING;
  }

  if (buttonPressed && gameState == START)
  {
    gameState = PLAYERS;
  }

  if (switchPressed && (gameState == END || gameState == END_MULTI))
  {
    resetFunc();
  }
}

void playerSelector() {
  if (joystickLeftPressed == true)
  {
    if (playerCount == 1) {
      playerCount = 4;
      playerSet = 4;
    }
    else {
      playerCount -= 1;
      playerSet -= 1;
    }
  }
  else if (joystickRightPressed == true)
  {
    playerCount = (playerCount % 4) + 1;
    playerSet = (playerSet % 4) + 1;
  }
}

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

  gameState = START;
  instruction = NONE;
  success = true;

  playerCount = 1;
  currentPlayer = 1;
  for (int i = 0; i < 4; i++) playerState[i] = true;

  instrCtr = 0;
  gameSpeed = 3050;
}

void loop() {
  delay(20);
  
  if (gameState == START) {
    static unsigned long lastBlinkTime = 0;

    changeScreen();
    
    if (millis() - lastBlinkTime > TEXT_BLINK_RATE)
    {
      lastBlinkTime = millis();
      textVisible = !textVisible;
    }

    display.clearDisplay();
    display.setCursor(24, 0);
    display.setTextSize(2);
    display.setTextColor(SH110X_WHITE);
    display.println(F("BOP IT!"));
    display.setCursor(8, 32);
    display.setTextSize(1);
    if (textVisible)
    {
      display.println(F("PRESS BTN TO START"));
    }
    display.display();
  }
  else if (gameState == PLAYERS) {
    static unsigned long lastBlinkTime = 0;

    changeScreen();
    playerSelector();
    if (millis() - lastBlinkTime > TEXT_BLINK_RATE)
    {
      lastBlinkTime = millis();
      textVisible = !textVisible;
    }

    display.clearDisplay();
    display.setCursor(24, 0);
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    display.println(F("N# OF PLAYERS"));

    display.fillTriangle(27, 32, 32, 34, 32, 30, SH110X_WHITE);
    display.fillTriangle(101, 32, 96, 34, 96, 30, SH110X_WHITE);

    display.setCursor(60, 24);
    display.setTextSize(2);
    display.setTextColor(SH110X_WHITE);
    display.println(playerSet);

    display.setCursor(8, 56);
    display.setTextSize(1);
    if (textVisible)
    {
      display.println(F("PRESS BTN TO START"));
    }
    display.display();
  }
  else if (gameState == ONGOING) {
    rgb_handler();
    if (playerSet == 1) tasker_solo();
    else tasker_multi();
    controller();
    decider();
  }
  else if (gameState == END) {
    analogWrite(R_LED, 0);
    analogWrite(G_LED, 0);

    changeScreen();

    static unsigned long lastBlinkTime2 = 0;
    if (millis() - lastBlinkTime2 > TEXT_BLINK_RATE)
    {
      lastBlinkTime2 = millis();
      textVisible = !textVisible;
    }

    display.clearDisplay();
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
  else {
    analogWrite(R_LED, 0);
    analogWrite(G_LED, 0);
    analogWrite(B_LED, 0);

    changeScreen();

    static unsigned long lastBlinkTime2 = 0;
    if (millis() - lastBlinkTime2 > TEXT_BLINK_RATE)
    {
      lastBlinkTime2 = millis();
      textVisible = !textVisible;
    }

    display.clearDisplay();
    display.setCursor(40, 0);
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    switch (currentPlayer) {
      case 1: display.println(F("PLAYER 1")); break;
      case 2: display.println(F("PLAYER 2")); break;
      case 3: display.println(F("PLAYER 3")); break;
      case 4: display.println(F("PLAYER 4")); break;
    }

    display.setCursor(32, 24);
    display.setTextSize(2);
    display.setTextColor(SH110X_WHITE);
    display.println(F("WINS!!"));

    display.setCursor(1, 56);
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    if (textVisible)
    {
      display.println(F("PRESS JOYSTICK SWITCH"));
    }
    display.display();
  }
}
