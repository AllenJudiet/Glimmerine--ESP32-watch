#include "AppConfig.h"
//snakegame.ino
#define SNAKE_CELL     10
#define SNAKE_COLS     30
#define SNAKE_ROWS     20
#define SNAKE_X        10
#define SNAKE_Y        22
#define SNAKE_MAX      (SNAKE_COLS * SNAKE_ROWS)
#define SNAKE_STEP_MS  135
#define SNAKE_INPUT_MS 90

int snakeX[SNAKE_MAX];
int snakeY[SNAKE_MAX];
int snakeLen = 0;
int snakeDir = 1;
int snakeNextDir = 1;
int foodX = 0, foodY = 0;
int snakeScore = 0;
bool snakeStarted = false;
bool snakeGameOver = false;
unsigned long lastSnakeStep = 0;
unsigned long lastSnakeInput = 0;

uint16_t C_SNAKE, C_SNAKE_HEAD, C_FOOD;

void initSnakeColours() {
  C_SNAKE      = tft.color565(80, 210, 150);
  C_SNAKE_HEAD = tft.color565(150, 255, 210);
  C_FOOD       = tft.color565(255, 210, 70);
}

void drawSnakeFrame() {
  tft.fillScreen(C_BG);
  tft.drawRoundRect(3, 3, SW - 6, SH - 6, 10, C_PUR);
  tft.drawRoundRect(SNAKE_X - 2, SNAKE_Y - 2,
                    SNAKE_COLS * SNAKE_CELL + 4,
                    SNAKE_ROWS * SNAKE_CELL + 4, 4, C_PUR_DIM);

  tft.setTextFont(2);
  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(C_PUR, C_BG);
  tft.drawString("SNAKE", 12, 12);
  tft.setTextDatum(MR_DATUM);
  tft.drawString(String(snakeScore), SW - 12, 12);
  tft.setTextDatum(TL_DATUM);
}

bool snakeHitsBody(int x, int y) {
  for (int i = 0; i < snakeLen; i++) {
    if (snakeX[i] == x && snakeY[i] == y) return true;
  }
  return false;
}

void placeFood() {
  do {
    foodX = random(SNAKE_COLS);
    foodY = random(SNAKE_ROWS);
  } while (snakeHitsBody(foodX, foodY));
}

void drawSnakeCell(int x, int y, uint16_t col) {
  tft.fillRect(SNAKE_X + x * SNAKE_CELL + 1,
               SNAKE_Y + y * SNAKE_CELL + 1,
               SNAKE_CELL - 2, SNAKE_CELL - 2, col);
}

void drawSnakeScore() {
  tft.fillRect(SW - 82, 5, 70, 16, C_BG);
  tft.setTextFont(2);
  tft.setTextDatum(MR_DATUM);
  tft.setTextColor(C_PUR, C_BG);
  tft.drawString(String(snakeScore), SW - 12, 12);
  tft.setTextDatum(TL_DATUM);
}

void drawSnakeGame() {
  drawSnakeFrame();
  drawSnakeCell(foodX, foodY, C_FOOD);
  for (int i = snakeLen - 1; i >= 0; i--) {
    drawSnakeCell(snakeX[i], snakeY[i], i == 0 ? C_SNAKE_HEAD : C_SNAKE);
  }
}

void showSnakeReady() {
  tft.fillRoundRect(58, 82, 204, 74, 6, tft.color565(8, 0, 18));
  tft.drawRoundRect(58, 82, 204, 74, 6, C_PUR);
  tft.setTextDatum(MC_DATUM);
  tft.setTextFont(4);
  tft.setTextColor(C_PUR, tft.color565(8, 0, 18));
  tft.drawString("SNAKE", SW / 2, 105);
  tft.setTextFont(2);
  tft.setTextColor(C_PUR_DIM, tft.color565(8, 0, 18));
  tft.drawString("TAP START", SW / 2, 135);
  tft.setTextDatum(TL_DATUM);
}

void startSnakeGame() {
  snakeStarted = true;
  lastSnakeStep = millis();
  drawSnakeGame();
}

void resetSnakeGame() {
  snakeLen = 4;
  snakeDir = 1;
  snakeNextDir = 1;
  snakeScore = 0;
  snakeStarted = false;
  snakeGameOver = false;
  lastSnakeStep = millis();
  lastSnakeInput = 0;

  int startX = SNAKE_COLS / 2;
  int startY = SNAKE_ROWS / 2;
  for (int i = 0; i < snakeLen; i++) {
    snakeX[i] = startX - i;
    snakeY[i] = startY;
  }

  placeFood();
  drawSnakeGame();
  showSnakeReady();
}

void showSnakeGameOver() {
  snakeGameOver = true;
  tft.fillRoundRect(58, 82, 204, 74, 6, tft.color565(8, 0, 18));
  tft.drawRoundRect(58, 82, 204, 74, 6, C_PUR);
  tft.setTextDatum(MC_DATUM);
  tft.setTextFont(4);
  tft.setTextColor(C_PUR, tft.color565(8, 0, 18));
  tft.drawString("GAME OVER", SW / 2, 105);
  tft.setTextFont(2);
  tft.setTextColor(C_PUR_DIM, tft.color565(8, 0, 18));
  tft.drawString("TAP RESTART", SW / 2, 135);
  tft.setTextDatum(TL_DATUM);
}

void showSnakeScreen() {
  currentScreen = SCREEN_SNAKE;
  initSnakeColours();
  randomSeed(micros());
  resetSnakeGame();
}

void renderSnakeSnapshot() {
  initSnakeColours();
  drawSnakeGame();
  if (snakeGameOver) showSnakeGameOver();
  else if (!snakeStarted) showSnakeReady();
}

void setSnakeDirection(int newDir) {
  if ((snakeDir == 0 && newDir == 2) || (snakeDir == 2 && newDir == 0)) return;
  if ((snakeDir == 1 && newDir == 3) || (snakeDir == 3 && newDir == 1)) return;
  snakeNextDir = newDir;
}

void readSnakeTouch() {
  unsigned long now = millis();

  if (touchPressed(TOUCH_SELECT)) {
    if (snakeGameOver) {
      resetSnakeGame();
      startSnakeGame();
    } else if (!snakeStarted) {
      startSnakeGame();
    }
  }

  if (now - lastSnakeInput < SNAKE_INPUT_MS || snakeGameOver || !snakeStarted) return;

  if (touchPressed(TOUCH_UP)) {
    setSnakeDirection(0);
    lastSnakeInput = now;
  } else if (touchPressed(TOUCH_RIGHT)) {
    setSnakeDirection(1);
    lastSnakeInput = now;
  } else if (touchPressed(TOUCH_DOWN)) {
    setSnakeDirection(2);
    lastSnakeInput = now;
  } else if (touchPressed(TOUCH_LEFT)) {
    setSnakeDirection(3);
    lastSnakeInput = now;
  }
}

void stepSnake() {
  if (snakeGameOver || !snakeStarted) return;

  snakeDir = snakeNextDir;
  int nx = snakeX[0];
  int ny = snakeY[0];

  if (snakeDir == 0) ny--;
  else if (snakeDir == 1) nx++;
  else if (snakeDir == 2) ny++;
  else nx--;

  if (nx < 0 || nx >= SNAKE_COLS || ny < 0 || ny >= SNAKE_ROWS || snakeHitsBody(nx, ny)) {
    showSnakeGameOver();
    return;
  }

  bool ate = (nx == foodX && ny == foodY);
  int oldTailX = snakeX[snakeLen - 1];
  int oldTailY = snakeY[snakeLen - 1];

  if (ate && snakeLen < SNAKE_MAX) snakeLen++;

  for (int i = snakeLen - 1; i > 0; i--) {
    snakeX[i] = snakeX[i - 1];
    snakeY[i] = snakeY[i - 1];
  }
  snakeX[0] = nx;
  snakeY[0] = ny;

  if (!ate) drawSnakeCell(oldTailX, oldTailY, C_BG);
  else {
    snakeScore++;
    drawSnakeScore();
    placeFood();
    drawSnakeCell(foodX, foodY, C_FOOD);
  }

  drawSnakeCell(snakeX[1], snakeY[1], C_SNAKE);
  drawSnakeCell(snakeX[0], snakeY[0], C_SNAKE_HEAD);
}

void updateSnakeScreen() {
  readSnakeTouch();
  if (currentScreen != SCREEN_SNAKE) return;

  unsigned long now = millis();
  if (now - lastSnakeStep >= SNAKE_STEP_MS) {
    lastSnakeStep = now;
    stepSnake();
  }
}
