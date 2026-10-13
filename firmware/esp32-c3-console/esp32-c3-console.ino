#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <LittleFS.h>

#define TFT_CS   7
#define TFT_RST  10
#define TFT_DC   6
#define TFT_MOSI 4
#define TFT_SCLK 5

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
HardwareSerial LinkSerial(1);

bool btnUp, btnDown, btnLeft, btnRight, btnA, btnB;
bool btnUpPrev, btnDownPrev, btnLeftPrev, btnRightPrev, btnAPrev, btnBPrev;
bool upEdge, downEdge, leftEdge, rightEdge, aEdge, bEdge;

enum AppState { HOME, GAMES_MENU, SETTINGS_MENU, GAME_SNAKE, PAUSE, BUTTON_TEST, GAME_2048 };
AppState state = HOME;

String rxBuffer = "";
int batteryPercent = -1; // -1 = not yet received
int gamesMenuIndex = 0;
int settingsMenuIndex = 0;

// ---------- Layout constants ----------
#define SCR_W 128
#define SCR_H 160
#define TOP_BAR_H 12
#define MARGIN 4
#define WALL_X MARGIN
#define WALL_Y (TOP_BAR_H + MARGIN)
#define WALL_W 120
#define WALL_H 140

#define GRID_SIZE 8
#define GRID_W 15
#define GRID_H 18
#define SCORE_BAR_H 12
#define BORDER 1

const int PLAY_X0 = (128 - GRID_W * GRID_SIZE) / 2;
const int PLAY_Y0 = SCORE_BAR_H + 4;
const int PLAY_W = GRID_W * GRID_SIZE;
const int PLAY_H = GRID_H * GRID_SIZE;

// ---------- Shared helpers ----------
void sendMotor(int ms) {
  LinkSerial.print("M:");
  LinkSerial.println(ms);
}
void sendTone(int freq, int ms) {
  LinkSerial.print("S:");
  LinkSerial.print(freq);
  LinkSerial.print(":");
  LinkSerial.println(ms);
}

void drawTopBar(const char* title, int score) {
  tft.fillRect(0, 0, 128, SCORE_BAR_H, ST77XX_BLACK);
  tft.drawFastHLine(0, SCORE_BAR_H - 1, 128, ST77XX_WHITE);
  tft.setTextSize(1);
  tft.setCursor(3, 2);
  tft.setTextColor(ST77XX_CYAN);
  tft.print(title);

  char buf[6];
  sprintf(buf, "%04d", score);
  int textW = 6 * 4;
  tft.setCursor(128 - 3 - textW, 2);
  tft.setTextColor(ST77XX_WHITE);
  tft.print(buf);
}

void drawPlayBorder() {
  tft.drawRect(PLAY_X0 - BORDER, PLAY_Y0 - BORDER,
               PLAY_W + BORDER * 2, PLAY_H + BORDER * 2, ST77XX_WHITE);
}

void setup() {
  Serial.begin(115200);
  LinkSerial.begin(115200, SERIAL_8N1, 20, 21);
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(0);
  tft.fillScreen(ST77XX_BLACK);
  delay(300);

  if (!LittleFS.begin()) {
    Serial.println("LittleFS mount failed");
  }

  randomSeed(analogRead(0));
  drawHome();
}

void loop() {
  readUART();
  updateEdges();

  switch (state) {
    case HOME:           handleHome(); break;
    case GAMES_MENU:      handleGamesMenu(); break;
    case SETTINGS_MENU:   handleSettingsMenu(); break;
    case GAME_SNAKE:      updateSnake(); break;
    case PAUSE:           handlePause(); break;
    case BUTTON_TEST:     handleButtonTest(); break;
    case GAME_2048:       update2048(); break;
  }
}

void readUART() {
  while (LinkSerial.available()) {
    char c = LinkSerial.read();
    if (c == '\n') {
      parseIncoming(rxBuffer);
      rxBuffer = "";
    } else {
      rxBuffer += c;
    }
  }
}

void parseIncoming(String msg) {
  msg.trim();
  if (msg.startsWith("B:")) {
    int sep = msg.indexOf(':', 2);
    if (sep < 0) return;
    String name = msg.substring(2, sep);
    bool pressed = msg.substring(sep + 1).toInt() == 1;

    if (name == "UP") btnUp = pressed;
    else if (name == "DOWN") btnDown = pressed;
    else if (name == "LEFT") btnLeft = pressed;
    else if (name == "RIGHT") btnRight = pressed;
    else if (name == "A") btnA = pressed;
    else if (name == "B") btnB = pressed;
  } else if (msg.startsWith("V:")) {
    int pct = msg.substring(2).toInt();
    batteryPercent = pct;
    if (state == HOME) drawBatteryBar();
  }
}

void updateEdges() {
  upEdge    = btnUp    && !btnUpPrev;
  downEdge  = btnDown  && !btnDownPrev;
  leftEdge  = btnLeft  && !btnLeftPrev;
  rightEdge = btnRight && !btnRightPrev;
  aEdge     = btnA     && !btnAPrev;
  bEdge     = btnB     && !btnBPrev;

  btnUpPrev = btnUp; btnDownPrev = btnDown;
  btnLeftPrev = btnLeft; btnRightPrev = btnRight;
  btnAPrev = btnA; btnBPrev = btnB;
}

// ---------- Home Screen ----------
int homeIndex = 0;
const char* homeItems[] = {"Games", "Settings"};
const int homeCount = 2;

void drawBatteryBar() {
  tft.fillRect(0, 0, 128, TOP_BAR_H, ST77XX_BLACK);
  tft.setTextSize(1);
  tft.setCursor(3, 2);
  tft.setTextColor(ST77XX_GREEN);
  tft.print("Battery: ");
  if (batteryPercent < 0) {
    tft.print("--");
  } else {
    tft.print(batteryPercent);
    tft.print("%");
  }
}

void drawWallpaper() {
  File f = LittleFS.open("/wallpaper.bin", "r");
  if (!f) {
    tft.fillRect(WALL_X, WALL_Y, WALL_W, WALL_H, ST77XX_BLUE);
    tft.setCursor(WALL_X + 5, WALL_Y + 60);
    tft.setTextColor(ST77XX_WHITE);
    tft.print("no wallpaper");
    return;
  }

  uint8_t rowBuf[WALL_W * 2];
  tft.startWrite();
  tft.setAddrWindow(WALL_X, WALL_Y, WALL_W, WALL_H);
  for (int y = 0; y < WALL_H; y++) {
    f.read(rowBuf, WALL_W * 2);
    tft.writePixels((uint16_t*)rowBuf, WALL_W, true, true); // bigEndian=true matches how we stored it
  }
  tft.endWrite();
  f.close();
}

void drawHome() {
  tft.fillScreen(ST77XX_BLACK);
  drawBatteryBar();
  drawWallpaper();
  drawHomeOverlay();
}

void drawHomeOverlay() {
  int overlayY = WALL_Y + WALL_H - 40;
  tft.fillRect(WALL_X, overlayY, WALL_W, 40, ST77XX_BLACK);
  tft.drawRect(WALL_X, overlayY, WALL_W, 40, ST77XX_WHITE);

  tft.setTextSize(1);
  for (int i = 0; i < homeCount; i++) {
    int y = overlayY + 8 + i * 16;
    tft.setCursor(WALL_X + 6, y);
    tft.setTextColor(i == homeIndex ? ST77XX_YELLOW : ST77XX_WHITE);
    if (i == homeIndex) tft.print("> ");
    else tft.print("  ");
    tft.println(homeItems[i]);
  }
}

void handleHome() {
  bool changed = false;
  if (downEdge) { homeIndex = (homeIndex + 1) % homeCount; changed = true; }
  if (upEdge)   { homeIndex = (homeIndex - 1 + homeCount) % homeCount; changed = true; }
  if (changed) {
    drawHomeOverlay();
    sendMotor(20);
  }
  if (aEdge) {
    if (homeIndex == 0) { gamesMenuIndex = 0; drawGamesMenu(); state = GAMES_MENU; }
    else if (homeIndex == 1) { settingsMenuIndex = 0; drawSettingsMenu(); state = SETTINGS_MENU; }
  }
}

// ---------- Games Menu ----------

const char* gamesMenuItems[] = {"Snake", "2048"};
const int gamesMenuCount = 2;

void drawGamesMenu() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize(1);
  tft.setCursor(10, 4);
  tft.setTextColor(ST77XX_CYAN);
  tft.println("== GAMES ==");
  tft.drawFastHLine(0, 16, 128, ST77XX_WHITE);
  for (int i = 0; i < gamesMenuCount; i++) {
    if (i == gamesMenuIndex) { tft.setCursor(4, 28 + i * 16); tft.setTextColor(ST77XX_YELLOW); tft.print(">"); }
    tft.setCursor(14, 28 + i * 16);
    tft.setTextColor(i == gamesMenuIndex ? ST77XX_YELLOW : ST77XX_WHITE);
    tft.println(gamesMenuItems[i]);
  }
  tft.setCursor(4, 140);
  tft.setTextColor(ST77XX_CYAN);
  tft.println("B: back to home");
}

void handleGamesMenu() {
  bool changed = false;
  if (downEdge) { gamesMenuIndex = (gamesMenuIndex + 1) % gamesMenuCount; changed = true; }
  if (upEdge)   { gamesMenuIndex = (gamesMenuIndex - 1 + gamesMenuCount) % gamesMenuCount; changed = true; }
  if (changed) { drawGamesMenu(); sendMotor(20); }
  if (aEdge) {
    if (gamesMenuIndex == 0) { startSnake(); state = GAME_SNAKE; }
    else if (gamesMenuIndex == 1) { start2048(); state = GAME_2048; }
  }
  if (bEdge) { homeIndex = 0; drawHome(); state = HOME; }
}

// ---------- Settings Menu ----------

const char* settingsMenuItems[] = {"Button Test"};
const int settingsMenuCount = 1;

void drawSettingsMenu() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize(1);
  tft.setCursor(10, 4);
  tft.setTextColor(ST77XX_CYAN);
  tft.println("== SETTINGS ==");
  tft.drawFastHLine(0, 16, 128, ST77XX_WHITE);
  for (int i = 0; i < settingsMenuCount; i++) {
    if (i == settingsMenuIndex) { tft.setCursor(4, 28 + i * 16); tft.setTextColor(ST77XX_YELLOW); tft.print(">"); }
    tft.setCursor(14, 28 + i * 16);
    tft.setTextColor(i == settingsMenuIndex ? ST77XX_YELLOW : ST77XX_WHITE);
    tft.println(settingsMenuItems[i]);
  }
  tft.setCursor(4, 140);
  tft.setTextColor(ST77XX_CYAN);
  tft.println("B: back to home");
}

void handleSettingsMenu() {
  bool changed = false;
  if (downEdge) { settingsMenuIndex = (settingsMenuIndex + 1) % settingsMenuCount; changed = true; }
  if (upEdge)   { settingsMenuIndex = (settingsMenuIndex - 1 + settingsMenuCount) % settingsMenuCount; changed = true; }
  if (changed) { drawSettingsMenu(); sendMotor(20); }
  if (aEdge) {
    if (settingsMenuIndex == 0) { startButtonTest(); state = BUTTON_TEST; }
  }
  if (bEdge) { homeIndex = 0; drawHome(); state = HOME; }
}

void handlePause() {
  if (aEdge) { state = GAME_SNAKE; }
  if (bEdge) { drawGamesMenu(); state = GAMES_MENU; }
}

// ---------- Button Test ----------
const char* btNames[6] = {"UP", "DOWN", "LEFT", "RIGHT", "A", "B"};
bool btLastDrawn[6];
unsigned long lastBPressTime = 0;
const unsigned long DOUBLE_PRESS_WINDOW = 400;

void startButtonTest() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize(1);
  tft.setCursor(10, 2);
  tft.setTextColor(ST77XX_CYAN);
  tft.println("Double-tap B to exit");
  for (int i = 0; i < 6; i++) btLastDrawn[i] = false;
  drawButtonBoxes(true);
}

void handleButtonTest() {
  if (bEdge) {
    unsigned long now = millis();
    if (now - lastBPressTime < DOUBLE_PRESS_WINDOW) {
      drawSettingsMenu();
      state = SETTINGS_MENU;
      return;
    }
    lastBPressTime = now;
  }
  drawButtonBoxes(false);
}

void drawButtonBoxes(bool force) {
  bool current[6] = {btnUp, btnDown, btnLeft, btnRight, btnA, btnB};
  for (int i = 0; i < 6; i++) {
    if (force || current[i] != btLastDrawn[i]) {
      int y = 20 + i * 22;
      tft.drawRect(10, y, 108, 18, ST77XX_WHITE);
      tft.fillRect(12, y + 2, 104, 14, current[i] ? ST77XX_GREEN : ST77XX_BLACK);
      tft.setCursor(20, y + 5);
      tft.setTextSize(1);
      tft.setTextColor(current[i] ? ST77XX_BLACK : ST77XX_WHITE);
      tft.println(btNames[i]);
      btLastDrawn[i] = current[i];
    }
  }
}

// ---------- Snake ----------
int snakeX[64], snakeY[64];
int snakeLen, dirX, dirY, foodX, foodY;
unsigned long lastMove;
int moveInterval = 200;
bool gameOver;
int score = 0;

void startSnake() {
  snakeLen = 3;
  for (int i = 0; i < snakeLen; i++) { snakeX[i] = 5 - i; snakeY[i] = 8; }
  dirX = 1; dirY = 0;
  placeFood();
  gameOver = false;
  moveInterval = 200;
  lastMove = millis();
  score = 0;
  tft.fillScreen(ST77XX_BLACK);
  drawTopBar("Snake", score);
  drawPlayBorder();
}

void placeFood() {
  bool onSnake;
  do {
    onSnake = false;
    foodX = random(0, GRID_W);
    foodY = random(0, GRID_H);
    for (int i = 0; i < snakeLen; i++) {
      if (snakeX[i] == foodX && snakeY[i] == foodY) { onSnake = true; break; }
    }
  } while (onSnake);
}

int cellX(int gx) { return PLAY_X0 + gx * GRID_SIZE; }
int cellY(int gy) { return PLAY_Y0 + gy * GRID_SIZE; }

void updateSnake() {
  if (upEdge    && dirY == 0) { dirX = 0; dirY = -1; }
  if (downEdge  && dirY == 0) { dirX = 0; dirY = 1; }
  if (leftEdge  && dirX == 0) { dirX = -1; dirY = 0; }
  if (rightEdge && dirX == 0) { dirX = 1; dirY = 0; }
  if (bEdge) { state = PAUSE; drawPauseOverlay(); return; }

  if (gameOver) { if (aEdge) startSnake(); return; }

  if (millis() - lastMove < moveInterval) return;
  lastMove = millis();

  int newX = snakeX[0] + dirX;
  int newY = snakeY[0] + dirY;

  if (newX < 0 || newX >= GRID_W || newY < 0 || newY >= GRID_H) { triggerGameOver(); return; }
  for (int i = 0; i < snakeLen; i++) {
    if (snakeX[i] == newX && snakeY[i] == newY) { triggerGameOver(); return; }
  }

  bool ate = (newX == foodX && newY == foodY);
  tft.fillRect(cellX(snakeX[snakeLen-1]), cellY(snakeY[snakeLen-1]), GRID_SIZE, GRID_SIZE, ST77XX_BLACK);

  for (int i = snakeLen - 1; i > 0; i--) { snakeX[i] = snakeX[i-1]; snakeY[i] = snakeY[i-1]; }
  snakeX[0] = newX; snakeY[0] = newY;

  if (ate) {
    snakeLen++;
    snakeX[snakeLen-1] = snakeX[snakeLen-2];
    snakeY[snakeLen-1] = snakeY[snakeLen-2];
    score += 10;
    drawTopBar("Snake", score);
    sendTone(1500, 60);
    sendMotor(30);
    placeFood();
    if (moveInterval > 80) moveInterval -= 5;
  }

  tft.fillRect(cellX(snakeX[0]), cellY(snakeY[0]), GRID_SIZE, GRID_SIZE, ST77XX_GREEN);
  tft.fillRect(cellX(foodX), cellY(foodY), GRID_SIZE, GRID_SIZE, ST77XX_RED);
}

void triggerGameOver() {
  gameOver = true;
  sendTone(200, 300);
  sendMotor(150);
  tft.setCursor(15, 90);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  tft.println("GAME OVER");
  tft.setCursor(10, 105);
  tft.println("A to retry");
}

void drawPauseOverlay() {
  tft.setCursor(25, 90);
  tft.setTextColor(ST77XX_YELLOW);
  tft.println("PAUSED");
  tft.setCursor(5, 105);
  tft.println("A resume, B menu");
}

// ---------- 2048 ----------
#define G2_SIZE 4
int grid2[G2_SIZE][G2_SIZE];
int score2048;
bool game2048Over;
const int cell2 = PLAY_W / G2_SIZE;
const int board2X = PLAY_X0;
const int board2Y = PLAY_Y0 + (PLAY_H - cell2 * G2_SIZE) / 2;

uint16_t tileColor(int v) {
  switch (v) {
    case 0: return ST77XX_BLACK;
    case 2: return 0xC618;
    case 4: return 0xFFF0;
    case 8: return ST77XX_ORANGE;
    case 16: return 0xFC00;
    case 32: return ST77XX_RED;
    case 64: return ST77XX_MAGENTA;
    case 128: return ST77XX_YELLOW;
    case 256: return ST77XX_GREEN;
    case 512: return ST77XX_CYAN;
    case 1024: return ST77XX_BLUE;
    default: return ST77XX_WHITE;
  }
}

void spawnTile2048() {
  int empties[16][2];
  int count = 0;
  for (int r = 0; r < G2_SIZE; r++)
    for (int c = 0; c < G2_SIZE; c++)
      if (grid2[r][c] == 0) { empties[count][0] = r; empties[count][1] = c; count++; }
  if (count == 0) return;
  int pick = random(0, count);
  grid2[empties[pick][0]][empties[pick][1]] = (random(0, 10) == 0) ? 4 : 2;
}

void start2048() {
  for (int r = 0; r < G2_SIZE; r++)
    for (int c = 0; c < G2_SIZE; c++)
      grid2[r][c] = 0;
  score2048 = 0;
  game2048Over = false;
  spawnTile2048();
  spawnTile2048();

  tft.fillScreen(ST77XX_BLACK);
  drawTopBar("2048", score2048);
  drawPlayBorder();
  draw2048Board();
}

void draw2048Board() {
  for (int r = 0; r < G2_SIZE; r++) {
    for (int c = 0; c < G2_SIZE; c++) {
      int x = board2X + c * cell2;
      int y = board2Y + r * cell2;
      int v = grid2[r][c];
      tft.fillRect(x + 1, y + 1, cell2 - 2, cell2 - 2, tileColor(v));
      tft.drawRect(x, y, cell2, cell2, ST77XX_WHITE);
      if (v != 0) {
        tft.setTextSize(1);
        tft.setTextColor(v <= 4 ? ST77XX_BLACK : ST77XX_WHITE);
        int tx = x + cell2 / 2 - (v < 10 ? 3 : (v < 100 ? 6 : 9));
        int ty = y + cell2 / 2 - 4;
        tft.setCursor(tx, ty);
        tft.print(v);
      }
    }
  }
}

bool slideRow(int row[G2_SIZE], bool reverse) {
  int vals[G2_SIZE];
  int n = 0;
  for (int i = 0; i < G2_SIZE; i++) {
    int idx = reverse ? (G2_SIZE - 1 - i) : i;
    if (row[idx] != 0) vals[n++] = row[idx];
  }
  bool changed = false;
  for (int i = 0; i < n - 1; i++) {
    if (vals[i] == vals[i+1]) {
      vals[i] *= 2;
      score2048 += vals[i];
      for (int j = i + 1; j < n - 1; j++) vals[j] = vals[j+1];
      n--;
      changed = true;
    }
  }
  int newRow[G2_SIZE] = {0,0,0,0};
  for (int i = 0; i < n; i++) {
    int idx = reverse ? (G2_SIZE - 1 - i) : i;
    newRow[idx] = vals[i];
  }
  for (int i = 0; i < G2_SIZE; i++) {
    if (row[i] != newRow[i]) changed = true;
    row[i] = newRow[i];
  }
  return changed;
}

bool move2048(int dx, int dy) {
  bool anyChanged = false;
  if (dx != 0) {
    for (int r = 0; r < G2_SIZE; r++) {
      int row[G2_SIZE];
      for (int c = 0; c < G2_SIZE; c++) row[c] = grid2[r][c];
      bool changed = slideRow(row, dx > 0);
      for (int c = 0; c < G2_SIZE; c++) grid2[r][c] = row[c];
      if (changed) anyChanged = true;
    }
  } else {
    for (int c = 0; c < G2_SIZE; c++) {
      int col[G2_SIZE];
      for (int r = 0; r < G2_SIZE; r++) col[r] = grid2[r][c];
      bool changed = slideRow(col, dy > 0);
      for (int r = 0; r < G2_SIZE; r++) grid2[r][c] = col[r];
      if (changed) anyChanged = true;
    }
  }
  return anyChanged;
}

bool has2048Moves() {
  for (int r = 0; r < G2_SIZE; r++)
    for (int c = 0; c < G2_SIZE; c++) {
      if (grid2[r][c] == 0) return true;
      if (c < G2_SIZE - 1 && grid2[r][c] == grid2[r][c+1]) return true;
      if (r < G2_SIZE - 1 && grid2[r][c] == grid2[r+1][c]) return true;
    }
  return false;
}

void update2048() {
  if (bEdge) { drawGamesMenu(); state = GAMES_MENU; return; }

  if (game2048Over) {
    if (aEdge) start2048();
    return;
  }

  bool moved = false;
  if (upEdge)    moved = move2048(0, -1);
  else if (downEdge)  moved = move2048(0, 1);
  else if (leftEdge)  moved = move2048(-1, 0);
  else if (rightEdge) moved = move2048(1, 0);

  if (moved) {
    spawnTile2048();
    drawTopBar("2048", score2048);
    draw2048Board();
    sendTone(600, 40);

    if (!has2048Moves()) {
      game2048Over = true;
      sendTone(200, 300);
      sendMotor(150);
      tft.setCursor(15, 90);
      tft.setTextColor(ST77XX_WHITE);
      tft.setTextSize(1);
      tft.println("GAME OVER");
      tft.setCursor(10, 105);
      tft.println("A to retry");
    }
  }
}