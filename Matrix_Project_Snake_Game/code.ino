#include "LedControl.h"
#include <LiquidCrystal.h>
#include <EEPROM.h>

// song
#define NOTE_E5  659
#define NOTE_FS5 740
#define NOTE_G5  784
#define NOTE_A5  880
#define NOTE_B5  988

const byte rs = 9;
const byte en = 8;
const byte d4 = 7;
const byte d5 = 6;
const byte d6 = 5;
const byte d7 = 4;

const byte buzzerPin = 2;

LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

const byte dinPin = 12;
const byte clockPin = 11;
const byte loadPin = 10;
const byte matrixSize = 8;

LedControl lc = LedControl(dinPin, clockPin, loadPin, 1);  //DIN, CLK, LOAD, No. DRIVER

int matrixBrightness;
int LCDbrightness;
byte aboutScrollText = 0;
byte HTPscrollText = 0;

const byte pinSW = 13;
const byte pinX = A0;
const byte pinY = A1;

byte upDownArrows[8] = {
  B00100,
  B01110,
  B11111,
  B00000,
  B00000,
  B11111,
  B01110,
  B00100
};
byte block[8] = {
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
};
const byte matrixHello[matrixSize] = {
  B10010101,
  B10010101,
  B10010001,
  B10010101,
  B11110101,
  B10010101,
  B10010100,
  B10010101
};
const byte matrixMenu[4][8] = {
  //how to play
  { 0b00000000,
    0b00000001,
    0b00000010,
    0b00000100,
    0b10001000,
    0b01010000,
    0b00100000,
    0b00000000 },
    //settings
  { 0b00000000,
    0b00001000,
    0b00101010,
    0b00011100,
    0b01110111,
    0b00011100,
    0b00101010,
    0b00001000 },
    //start game
  { 0b00100000,
    0b00110000,
    0b00111000,
    0b00111100,
    0b00111100,
    0b00111000,
    0b00110000,
    0b00100000 },
    //about
  { 0b00000000,
    0b00011000,
    0b00100100,
    0b00000100,
    0b00001000,
    0b00001000,
    0b00000000,
    0b00001000  }
};
const byte matrixHappy[8] = {
  0b00000000,
  0b00100100,
  0b00100100,
  0b00100100,
  0b00000000,
  0b10000001,
  0b01000010,
  0b00111100
};
byte matrix[matrixSize][matrixSize] = {
  { 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0 }
};
const String menuOptions[4] = {
  "How to play",
  "Settings",
  "Start game",
  "About"
};
const String settingsOptions[5] = {
  "Player name",
  "LCD bright",
  "Matrix bright",
  "Sounds",
  "Reset HS"
};

String name = "AAA";
char nameLetters[3] = {
  'A', 'A', 'A'
};

byte namePos = 0;
byte menuCurrentItem = 0;

bool swState = LOW;
bool lastSwState = LOW;
byte state = 0;
byte switchState = HIGH;
int xValue = 0;
byte subMenuOption = 0;
int yValue = 0;

bool joyBackToMiddleX = LOW;
bool joyBackToMiddleY = LOW;

int minThreshold = 400;
int maxThreshold = 600;

unsigned long lastDebounceTime = 0;
unsigned long prevScrollTime = 0;
unsigned int debounceDelay = 50;
bool matrixChanged = true;
unsigned int score = 0;
byte xPos = 0;
byte yPos = 0;
byte xLastPos = 0;
byte yLastPos = 0;
byte lastFoodRow = 0;
byte lastFoodColumn = 0;
int scrollTextPosition = 0;
byte currentFoodRow = 0;
byte settings = 0;
byte beatHighscore = 0;
byte settingsPos = 0;
byte currentFoodColumn = 0;
byte startGame = 0;
String messageAbout = "Made by Ojoc DC. Github user:  CristianaOD.";
String messageHTP = "Eat the blinking food to increase the score";
const byte moveInterval = 100;

byte sounds;
unsigned long long lastBlink = 0;
byte blinkLetter = LOW;
const int blinkInterval = 250;
unsigned long long lastMoved = 0;
int highscores[5];
String highscoreNames[1] = {""};
unsigned long lastLetterBlink = 0;

unsigned long eatStartTime = 0;
int bestScore = 0;

const int MAX_SNAKE_SIZE = 16;  // dimensiunea maxima a sarpelui
int snakeSize = 1;  // lungimea initiala a sarpelui

struct SnakeSegment {
  int x;
  int y;
};

SnakeSegment snakeSegments[MAX_SNAKE_SIZE];  // pentru stocarea coordonatelor segmentelor sarpelui


void setup() {
  LCDbrightness = EEPROM.read(1);
  matrixBrightness = EEPROM.read(2);
  sounds = EEPROM.read(3);
  EEPROM.get(0, bestScore);
  song();
  analogWrite(3, LCDbrightness);
  pinMode(pinSW, INPUT_PULLUP);
  Serial.begin(9600);
  lc.shutdown(0, false);
  lc.setIntensity(0, matrixBrightness);
  lc.clearDisplay(0);
  for (int row = 0; row < matrixSize; row++) {
    lc.setRow(0, row, matrixHello[row]);
  }
  
  displayWelcomeMessage();
  lcd.begin(16, 2);
  lcd.print("     SNAKE");
  lcd.setCursor(0, 1);
  lcd.print(" Press to begin");
  lcd.createChar(0, upDownArrows);
  lcd.createChar(1, block);
}

void loop() {
  if (startGame == 1) {
    game();
    if (score == 10) {
      exitGame();
    }
  } 
  else {
    blinkLetterName();
    if (aboutScrollText == 1 && scrollTextPosition < messageAbout.length() - 15) {
      lcd.setCursor(0, 1);
      lcd.print(messageAbout.substring(scrollTextPosition-1, 16 + scrollTextPosition));
      if (millis() - prevScrollTime >= 700) {
        prevScrollTime = millis();
        lcd.setCursor(0, 1);
        lcd.print(messageAbout.substring(scrollTextPosition, 16 + scrollTextPosition));
        scrollTextPosition++;
      }
    }
    if (HTPscrollText == 1 && scrollTextPosition < messageAbout.length() - 15) {
      lcd.setCursor(0, 0);
      lcd.print("   HOW TO PLAY   ");
      lcd.setCursor(0, 1);
      lcd.print(messageHTP.substring(scrollTextPosition-1, 16 + scrollTextPosition));
      if (millis() - prevScrollTime >= 700) {
        prevScrollTime = millis();
        lcd.setCursor(0, 1);
        lcd.print(messageHTP.substring(scrollTextPosition, 16 + scrollTextPosition));
        scrollTextPosition++;
      }
    }
    swState = digitalRead(pinSW);
    xValue = analogRead(pinX);
    yValue = analogRead(pinY);
    buttonLogic();
    yAxisLogic();
    xAxisLogic();
  }
}

// melodia de inceput
void song() {
  int melody[] = {
    NOTE_E5, NOTE_FS5, NOTE_G5, NOTE_A5, NOTE_B5, NOTE_A5, NOTE_G5, NOTE_FS5, 0
  };
  int noteDurations[] = {
    4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4
  };
  for (int thisNote = 0; thisNote < 16; thisNote++) {
    int noteDuration = 1000 / noteDurations[thisNote];
    tone(buzzerPin, melody[thisNote], noteDuration);
    int pauseBetweenNotes = noteDuration * 1.30;
    delay(pauseBetweenNotes);
    noTone(buzzerPin);
  }
}

void displayWelcomeMessage() {
  lcd.begin(16, 2);
  lcd.print("    Welcome!");
  delay(2000);
  lcd.clear();
}

// logica jocului
void game() {
  if (millis() - lastBlink > blinkInterval) {
    matrix[currentFoodRow][currentFoodColumn] = !matrix[currentFoodRow][currentFoodColumn]; // clipit hrana
    updateMatrix(); // matricea dupa schimbari
    lastBlink = millis();
  }
  if (millis() - lastMoved > moveInterval) {
    updatePositions(); // pozitia sarpelui si a hranei
    lastMoved = millis();
  }
  if (matrixChanged == true) {
    updateMatrix();
    matrixChanged = false;
  }
}

void updateMatrix() {
  for (int row = 0; row < matrixSize; row++) {
    for (int col = 0; col < matrixSize; col++) {
      lc.setLed(0, row, col, matrix[row][col]);
    }
  }
  //parcurg segmentele sarpelui și actualizez matricea
  for (int i = 1; i < snakeSize; ++i) {
    int x = snakeSegments[i].x;
    int y = snakeSegments[i].y;
    matrix[x][y] = 1;
  }
}

// jocul s-a terminat
void exitGame() {
  for (int row = 0; row < matrixSize; row++) {
    for (int col = 0; col < matrixSize; col++) {
      lc.setLed(0, row, col, true);
    }
  }
  lcd.clear();
  menuCurrentItem = 0;
  subMenuOption = 0;
  startGame = 0;
  lcd.print("Congrats, ");
  lcd.print(nameLetters[0]);
  lcd.print(nameLetters[1]);
  lcd.print(nameLetters[2]);
  lcd.print("!");
  lcd.setCursor(0, 1);
  lcd.print("Your score: ");
  lcd.print(score);
  delay(1000);

  playVictoryMelody();

  for (int row = 0; row < matrixSize; row++) {
    lc.setRow(0, row, matrixHappy[row]);
  }
  delay(2000);
  // actualizeaza cel mai bun scor
  if (score > bestScore) {
    bestScore = score;
    lcd.setCursor(0, 3);
    lcd.print("New Highscore!");
    beatHighscore = 1;  // indicator pentru un nou scor maxim
    EEPROM.put(4, bestScore);
  }
  else {
    state = 1;
    lcd.clear();
    lcd.print("   MENU");
    lcd.setCursor(0, 1);
    lcd.print(">");
    lcd.print(menuOptions[menuCurrentItem]);
    lc.clearDisplay(0);
    for (int row = 0; row < matrixSize; row++) {
      lc.setRow(0, row, matrixMenu[menuCurrentItem][row]);
    }
  }
  score = 0;
}

// actualizeaza pozitia sarpelui
void updatePositions() {
  int xValue = analogRead(pinX);
  int yValue = analogRead(pinY);

  xLastPos = xPos;
  yLastPos = yPos;
 
  // ma misc spre dreapta
  if (xValue < minThreshold) { 
    if (xPos < matrixSize - 1) { // nu este la margine
      xPos++;
    } 
    else {
      xPos = 0;
    }
  }
  // ma misc spre stanga
  if (xValue > maxThreshold) {
    if (xPos > 0) {
      xPos--;
    } 
    else {
      xPos = matrixSize - 1;
    }
  }

  if (yValue > maxThreshold) {
    if (yPos < matrixSize - 1) {
      yPos++;
    } 
    else {
      yPos = 0;
    }
  }

  if (yValue < minThreshold) {
    if (yPos > 0) {
      yPos--;
    } 
    else {
      yPos = matrixSize - 1;
    }
  }
  if (xPos != xLastPos || yPos != yLastPos) { // daca sarpele s-a miscat din pozitia initiala
    if (xPos == currentFoodRow && yPos == currentFoodColumn) { // daca sarpele a ajuns la pozitia hranei
      score++;
      lcd.setCursor(7, 1);
      lcd.print(score);
      generateFood();
    }
    
    eatStartTime = millis();  // seteaza timpul de inceput
    matrixChanged = true;
    matrix[xLastPos][yLastPos] = 0; // sterg pozitia anterioara a sarpelui
    matrix[xPos][yPos] = 1;
  }

  //calculateSnake();
  // coordonatele sarpelui
  //for (int i = snakeSize - 1; i > 0; --i) {
  //  snakeSegments[i] = snakeSegments[i - 1];
  //}
  // seteaza noile coordonate ale capului sarpelui
  //snakeSegments[0].x = xPos;
  //snakeSegments[0].y = yPos;
}
// generez mancare (led-uri care clipesc)
void generateFood() {
  lastFoodRow = currentFoodRow;
  lastFoodColumn = currentFoodColumn;
  currentFoodRow = random(0, 8);
  currentFoodColumn = random(0, 8);
  matrix[lastFoodRow][lastFoodColumn] = 0;
  matrix[currentFoodRow][currentFoodColumn] = 1;
  matrixChanged = true;
}

// melodie de final
void playVictoryMelody() {
  int soundFrequency = 1000;  //Hz
  int soundDuration = 2000; 

  tone(buzzerPin, soundFrequency, soundDuration);
  delay(soundDuration);
  noTone(buzzerPin);
}

// litera curenta clipeste atunci cand jucatorul isi alege numele
void blinkLetterName() {
  if (state == 3 && settings == 1 && settingsPos == 0) {
    if (millis() - lastLetterBlink > 450) {
      lastLetterBlink = millis();
      blinkLetter = !blinkLetter; // inversez starea de clipire (HIGH sau LOW)
    }
    lcd.setCursor(namePos + 4, 1);
    if (blinkLetter == HIGH) {
      lcd.print(nameLetters[namePos]);
    } 
    else {
      lcd.print(" ");
    }
  }
}

void buttonLogic() {
  if (swState != lastSwState) {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (swState != switchState) {
      switchState = swState;
      if (switchState == LOW) {
        if (state == 0 && startGame == 0 || state == 4) { 
          state = 1;
          lcd.clear();
          lcd.print("     MENU");
          lcd.setCursor(15, 0);
          lcd.write(byte(0));
          lcd.setCursor(0, 1);
          lcd.print(">");
          lcd.print(menuOptions[menuCurrentItem]);
          lc.clearDisplay(0);
          for (int row = 0; row < matrixSize; row++) {
            lc.setRow(0, row, matrixMenu[menuCurrentItem][row]);
          }
        } 
        // jocul a inceput
        else if (subMenuOption == 1) { 
          startGame = 1;
          lcd.clear();
          unsigned long currentTime = millis();
          lcd.print("PLAYER: ");
          lcd.print(nameLetters[0]);
          lcd.print(nameLetters[1]);
          lcd.print(nameLetters[2]);
          lcd.setCursor(0, 1);
          lcd.print("SCORE: ");
          lcd.print(score);
          lcd.print("TIME: ");
          unsigned long elapsedTime = currentTime - eatStartTime;
          lcd.print(elapsedTime / 1000);  // afiseaza timpul in secunde
          lc.clearDisplay(0);
          matrix[xPos][yPos] = 1;
          generateFood();
          
        } 
        // jucatorul a ales numele
        else if (state == 3 && startGame == 0 && settings == 1 && settingsPos == 0) {  
          state = 2;
          lcd.clear();
          lcd.print("    SETTINGS");
          lcd.setCursor(15, 0);
          lcd.write(byte(0));
          lcd.setCursor(0, 1);
          lcd.print(">");
          lcd.print(settingsOptions[settingsPos]);
          lc.clearDisplay(0);
          for (int row = 0; row < matrixSize; row++) {
            lc.setRow(0, row, matrixMenu[menuCurrentItem][row]);
          }
        }
        else if (state == 3 && startGame == 0 && settings == 1 && settingsPos == 5) {  
          state = 2;
          lcd.clear();
          lcd.print("    SETTINGS");
          lcd.setCursor(15, 0);
          lcd.write(byte(0));
          lcd.setCursor(0, 1);
          lcd.print(">");
          lcd.print(settingsOptions[settingsPos]);
          lc.clearDisplay(0);
          for (int row = 0; row < matrixSize; row++) {
            lc.setRow(0, row, matrixMenu[menuCurrentItem][row]);
          }
        }
        
      }
    }
  }
  lastSwState = swState; // starea curenta a butonului 
}

void yAxisLogic() {
  // daca joystick-ul a fost miscat in sus si nu a revenit in pozitia centrala; suntem in menu si jocul nu a inceput inca
  if (yValue > maxThreshold && joyBackToMiddleY == LOW && state == 1 && startGame == 0) {  
    state = 2; // alegerea din meniu
    if (menuCurrentItem == 0) {
      lcd.clear();
      lcd.print("   HOW TO PLAY");
      lcd.setCursor(0, 1); // linia 2 si coloana 0
      HTPscrollText = 1; // textul trebuie să înceapă să se deruleze pe ecran
    } 
    else if (menuCurrentItem == 1) {
      lcd.clear();
      lcd.print("   SETTINGS");
      lcd.setCursor(15, 0);
      lcd.write(byte(0));
      settings = 1; // controleaz meniul de setari
      lcd.setCursor(0, 1); //linia 1 coloana 0
      lcd.print("                ");
      lcd.setCursor(0, 1);
      lcd.print(">");
      lcd.print(settingsOptions[settingsPos]);
    } 
    else if (menuCurrentItem == 2) {
      subMenuOption = 1;
      lcd.clear();
      lcd.print("   START GAME");
      lcd.setCursor(0, 1);
      lcd.print("Press to start");
    }  
    else if (menuCurrentItem == 3) {
      lcd.clear();
      lcd.print("      ABOUT");
      lcd.setCursor(0, 1);
      aboutScrollText = 1; // incepe derularea textului despre joc
    }
    joyBackToMiddleY = HIGH; //revine la pozitia centrala
  } 
  // joystick-ul este miscat in jos si nu a revenit in pozitia centrala; suntem in submeniu si jocul nu a inceput inca
  // din submeniu ma duc in meniu
  else if (yValue < minThreshold && joyBackToMiddleY == LOW && state == 2 && startGame == 0) { 
    state = 1;
    aboutScrollText = 0;
    HTPscrollText = 0;
    scrollTextPosition = 0;
    subMenuOption = 0;
    settingsPos = 0;
    settings = 0;
    lcd.clear();
    lcd.print("     MENU");
    lcd.setCursor(15, 0);
    lcd.write(byte(0));
    lcd.setCursor(0, 1);
    lcd.print(">");
    lcd.print(menuOptions[menuCurrentItem]);
    lc.clearDisplay(0);
    for (int row = 0; row < matrixSize; row++) { // afisez animatiile de pe matrice
      lc.setRow(0, row, matrixMenu[menuCurrentItem][row]);
    }
    joyBackToMiddleY = HIGH;
  } 
  // sunt in meniul de setari
  else if (yValue > maxThreshold && joyBackToMiddleY == LOW && state == 2 && startGame == 0 && settings == 1) { 
    state = 3;
    lcd.clear();
    if (settingsPos == 0) {
      lcd.print("<NAME>  Press to");
      lcd.setCursor(0, 1);
      lcd.write(byte(0));
      lcd.print("   ");
      lcd.print(nameLetters[0]);
      lcd.print(nameLetters[1]);
      lcd.print(nameLetters[2]);
      lcd.print("     SAVE");
    } 
    else if (settingsPos == 1) {
      lcd.print("<LCD BRIGHT>");
      lcd.setCursor(0, 1);
      lcd.write(byte(0));
      lcd.setCursor(4, 1);
      lcd.print("-");
      for (int i = 0; i < LCDbrightness; ++i)
        lcd.write(byte(1));
      lcd.setCursor(10, 1);
      lcd.print("+");
    } 
    else if (settingsPos == 2) {
      lcd.print("<MATRIX BRIGHT>");
      for (int row = 0; row < matrixSize; row++) {
        for (int col = 0; col < matrixSize; col++) {
          lc.setLed(0, row, col, true);
        }
      }
      lcd.setCursor(0, 1);
      lcd.write(byte(0));
      lcd.setCursor(4, 1);
      lcd.print("-");
      lcd.setCursor(5, 1);
      //for (int i = 0; i < matrixBrightness; ++i)
      //  lcd.write(byte(1));
      lcd.setCursor(10, 1);
      lcd.print("+");
    } 
    else if (settingsPos == 3) {
      lcd.print("<SOUNDS>");
      lcd.setCursor(0, 1);
      lcd.write(byte(0));
      lcd.print(" ");
      if (sounds == 1)
        lcd.print("ON");
      else
        lcd.print("OFF");
    }
    else if (settingsPos == 4) {
      lcd.print("<RESET HS>");
      lcd.setCursor(0, 1);
      lcd.print("Press to reset");
    }
    joyBackToMiddleY = HIGH;
  } 
  // ies din settings submeniu
  else if (yValue < minThreshold && joyBackToMiddleY == LOW && state == 3 && startGame == 0 && settings == 1 and settingsPos != 10) {
    state = 2;
    lcd.clear();
    lcd.print("   SETTINGS");
    lcd.setCursor(15, 0);
    lcd.write(byte(0));
    lcd.setCursor(0, 1);
    lcd.print(">");
    lcd.print(settingsOptions[settingsPos]);
    lc.clearDisplay(0);
    for (int row = 0; row < matrixSize; row++) {
      lc.setRow(0, row, matrixMenu[menuCurrentItem][row]);
    }
    joyBackToMiddleY = HIGH;
  } 
  // deplasez in stanga prin nume
  else if (yValue < minThreshold && joyBackToMiddleY == LOW && state == 3 && startGame == 0 && settings == 1 and settingsPos == 0) { 
    if (namePos > 0) {
      lcd.setCursor(namePos + 4, 1);
      lcd.print(nameLetters[namePos]);
      namePos--;
    }
    joyBackToMiddleY = HIGH;
  } 
  // deplasez in dreapta prin nume
  else if (yValue > maxThreshold && joyBackToMiddleY == LOW && state == 3 && startGame == 0 && settings == 1 and settingsPos == 0) { 
    if (namePos < 2) {
      lcd.setCursor(namePos + 4, 1);
      lcd.print(nameLetters[namePos]);
      namePos++;
    }
    joyBackToMiddleY = HIGH;
  } 
  //intoarcerea joystick-ului la pozitia de pornire
  else if (joyBackToMiddleY == HIGH && yValue < maxThreshold && yValue > minThreshold && startGame == 0) {
    joyBackToMiddleY = LOW;
  }
}

void xAxisLogic() {
  if (xValue < minThreshold && joyBackToMiddleX == LOW && state == 1 && startGame == 0){ //ma misc prin meniu
    if (menuCurrentItem < 3) {
      menuCurrentItem++;
      lcd.setCursor(0, 1);
      lcd.print("                ");
      lcd.setCursor(0, 1);
      lcd.print(">");
      lcd.print(menuOptions[menuCurrentItem]);
      lc.clearDisplay(0);
      for (int row = 0; row < matrixSize; row++) {
        lc.setRow(0, row, matrixMenu[menuCurrentItem][row]);
      }
    } 
    else if (menuCurrentItem == 3) {
      menuCurrentItem = 0;
      lcd.setCursor(0, 1);
      lcd.print("                ");
      lcd.setCursor(0, 1);
      lcd.print(">");
      lcd.print(menuOptions[menuCurrentItem]);
      lc.clearDisplay(0);
      for (int row = 0; row < matrixSize; row++) {
        lc.setRow(0, row, matrixMenu[menuCurrentItem][row]);
      }
    }
    joyBackToMiddleX = HIGH;
  } 
  else if (xValue > maxThreshold && joyBackToMiddleX == LOW && state == 1 && startGame == 0){ //ma misc in sus prin meniu
    if (menuCurrentItem > 0) {
      menuCurrentItem--;
      lcd.setCursor(0, 1);
      lcd.print("                ");
      lcd.setCursor(0, 1);
      lcd.print(">");
      lcd.print(menuOptions[menuCurrentItem]);
      lc.clearDisplay(0);
      for (int row = 0; row < matrixSize; row++) {
        lc.setRow(0, row, matrixMenu[menuCurrentItem][row]);
      }
    } 
    else if (menuCurrentItem == 0) {
      menuCurrentItem = 3;
      lcd.setCursor(0, 1);
      lcd.print("                ");
      lcd.setCursor(0, 1);
      lcd.print(">");
      lcd.print(menuOptions[menuCurrentItem]);
      lc.clearDisplay(0);
      for (int row = 0; row < matrixSize; row++) {
        lc.setRow(0, row, matrixMenu[menuCurrentItem][row]);
      }
    }
    joyBackToMiddleX = HIGH;
  } 
  else if (xValue < minThreshold && joyBackToMiddleX == LOW && state == 2 && startGame == 0 && settings == 1) {  // ma misc in jos prin submeniu
    if (settingsPos < 4) {
      settingsPos++;
      lcd.setCursor(0, 1);
      lcd.print("                ");
      lcd.setCursor(0, 1);
      lcd.print(">");
      lcd.print(settingsOptions[settingsPos]);
    }
    else if (settingsPos == 4) {
      settingsPos = 0;
      lcd.setCursor(0, 1);
      lcd.print("                ");
      lcd.setCursor(0, 1);
      lcd.print(">");
      lcd.print(settingsOptions[settingsPos]);
    }
    joyBackToMiddleX = HIGH;
  } 
  else if (xValue > maxThreshold && joyBackToMiddleX == LOW && state == 2 && startGame == 0 && settings == 1) {  // ma misc in sus prin submeniu
    if (settingsPos > 0) {
      settingsPos--;
      lcd.setCursor(0, 1);
      lcd.print("                ");
      lcd.setCursor(0, 1);
      lcd.print(">");
      lcd.print(settingsOptions[settingsPos]);
    }
    else if (settingsPos == 0) {
      settingsPos = 4;
      lcd.setCursor(0, 1);
      lcd.print("                ");
      lcd.setCursor(0, 1);
      lcd.print(">");
      lcd.print(settingsOptions[settingsPos]);
    }
    joyBackToMiddleX = HIGH;
  } 
  else if (xValue < minThreshold && joyBackToMiddleX == LOW && state == 3 && startGame == 0 && settings == 1) { 
    if (settingsPos == 0) {        // derulez prin litere cand este introdus numele                                                                                
      if (nameLetters[namePos] > 'A') {
        nameLetters[namePos]--;
        lcd.setCursor(namePos + 4, 1);
        lcd.print(nameLetters[namePos]);
      } 
      else if (nameLetters[namePos] == 'A') {
        nameLetters[namePos] = 'Z';
        lcd.setCursor(namePos + 4, 1);
        lcd.print(nameLetters[namePos]);
      }
    } 
    //modific luminozitatea LCD
    else if (settingsPos == 1) { 
      if (LCDbrightness > 1) {
        LCDbrightness--;
        lcd.setCursor(0, 1);
        lcd.print("                ");
        lcd.setCursor(0, 1);
        lcd.write(byte(0));
        lcd.setCursor(4, 1);
        lcd.print("-");
        for (int i = 0; i < LCDbrightness; ++i)
          lcd.write(byte(1));
        lcd.setCursor(10, 1);
        lcd.print("+");
        analogWrite(3, LCDbrightness);
        EEPROM.update(1, LCDbrightness);
      }
    } 
    // modific luminozitatea matrice
    else if (settingsPos == 2) { 
      if (matrixBrightness > 1) {
        matrixBrightness--;
        lcd.setCursor(0, 1);
        lcd.print("                ");
        lcd.setCursor(0, 1);
        lcd.write(byte(0));
        lcd.setCursor(4, 1);
        lcd.print("-");
        lcd.setCursor(5, 1);
        for (int i = 0; i < matrixBrightness; ++i)
          lcd.write(byte(1));
        lcd.setCursor(10, 1);
        lcd.print("+");
        lc.setIntensity(0, matrixBrightness);
        EEPROM.update(2, matrixBrightness);
      }
    } 
    // schimb sunet on/off
    else if (settingsPos == 3) { 
      if (sounds == 1) {
        sounds = 0;
        lcd.setCursor(0, 1);
        lcd.print("                ");
        lcd.setCursor(0, 1);
        lcd.write(byte(0));
        lcd.print(" ");
        lcd.print("OFF");
        EEPROM.update(3, sounds);
      }
    }
    joyBackToMiddleX = HIGH;
  } 
  else if (xValue > maxThreshold && joyBackToMiddleX == LOW && state == 3 && startGame == 0 && settings == 1) {
    if (settingsPos == 0) {
      if (nameLetters[namePos] < 'Z') {
        nameLetters[namePos]++;
        lcd.setCursor(namePos + 4, 1);
        lcd.print(nameLetters[namePos]);
      } 
      else if (nameLetters[namePos] == 'Z') {
        nameLetters[namePos] = 'A';
        lcd.setCursor(namePos + 4, 1);
        lcd.print(nameLetters[namePos]);
      }
    } 
    else if (settingsPos == 1) {
      if (LCDbrightness < 5) {
        LCDbrightness++;
        lcd.setCursor(0, 1);
        lcd.print("                ");
        lcd.setCursor(0, 1);
        lcd.write(byte(0));
        lcd.setCursor(4, 1);
        lcd.print("-");
        for (int i = 0; i < LCDbrightness; ++i)
          lcd.write(byte(1));
        lcd.setCursor(10, 1);
        lcd.print("+");
        analogWrite(3, LCDbrightness);
        EEPROM.update(1, LCDbrightness);
      }
    } 
    else if (settingsPos == 2) {
      if (matrixBrightness < 5) {
        matrixBrightness++;
        lcd.setCursor(0, 1);
        lcd.print("                ");
        lcd.setCursor(0, 1);
        lcd.write(byte(0));
        lcd.setCursor(4, 1);
        lcd.print("-");
        lcd.setCursor(5, 1);
        for (int i = 0; i < matrixBrightness; ++i)
          lcd.write(byte(1));
        lcd.setCursor(10, 1);
        lcd.print("+");
        lc.setIntensity(0, matrixBrightness);
        EEPROM.update(2, matrixBrightness);
      }
    } 
    else if (settingsPos == 3) {
      if (sounds == 0) {
        sounds = 1;
        lcd.setCursor(0, 1);
        lcd.print("                ");
        lcd.setCursor(0, 1);
        lcd.write(byte(0));
        lcd.print(" ");
        lcd.print("ON");
        EEPROM.update(3, sounds);
      }
    }
    joyBackToMiddleX = HIGH;
  } 
  else if (joyBackToMiddleX == HIGH && xValue < maxThreshold && xValue > minThreshold && startGame == 0) {
    joyBackToMiddleX = LOW;
  }
}
