#include "apps/SnakeApp.h"
#include "core/Scores.h"
#include "core/Mood.h"

static const uint16_t SNAKE_BODY = 0x07E0;
static const uint16_t SNAKE_HEAD = COL_EYE;
static const uint16_t FOOD_COL   = COL_PINK;
static const uint16_t BORDER     = 0x2124;

void SnakeApp::enter(TFT_eSPI& tft) {
  g = &tft;
  best = Scores::best("snake");
  needsReset = false;
  if (!started || over) {
    reset();
  } else {
    // Coming back to a game in progress: show it paused
    paused = true;
    drawBoard();
    banner("Paused", "tap to continue");
  }
}

void SnakeApp::reset() {
  len = 4;
  for (int i = 0; i < len; i++) body[i] = {(int8_t)(8 - i), (int8_t)(ROWS / 2)};
  dir = nextDir = 1;
  score = 0;
  over = false;
  paused = false;
  started = true;
  placeFood();
  drawBoard();
  lastStep = millis();
}

void SnakeApp::drawBoard() {
  g->fillScreen(COL_BG);
  g->drawRect(0, TOP - 1, SCREEN_W, ROWS * CELL + 2, BORDER);
  for (int i = len - 1; i >= 0; i--) cell(body[i], i == 0 ? SNAKE_HEAD : SNAKE_BODY);
  drawFood();
  drawScore();
}

void SnakeApp::cell(P p, uint16_t c) {
  g->fillRoundRect(p.x * CELL + 1, TOP + p.y * CELL + 1, CELL - 2, CELL - 2, 4, c);
}

void SnakeApp::drawFood() {
  g->fillCircle(food.x * CELL + CELL / 2, TOP + food.y * CELL + CELL / 2, CELL / 2 - 2, FOOD_COL);
}

void SnakeApp::drawScore() {
  g->fillRect(0, 0, SCREEN_W, TOP - 1, COL_BG);
  g->setTextDatum(TL_DATUM);
  g->setTextColor(COL_FG, COL_BG);
  g->drawString("Score " + String(score), 4, 0, 2);
  g->setTextDatum(TR_DATUM);
  g->setTextColor(COL_DIM, COL_BG);
  g->drawString("Best " + String(best) + "   hold = menu", SCREEN_W - 4, 0, 2);
}

void SnakeApp::banner(const char* big, const char* small) {
  g->fillRoundRect(60, 82, 200, 74, 10, COL_PANEL);
  g->drawRoundRect(60, 82, 200, 74, 10, COL_EYE);
  g->setTextDatum(MC_DATUM);
  g->setTextColor(COL_FG);
  g->drawString(big, SCREEN_W / 2, 106, 4);
  g->setTextColor(COL_DIM);
  g->drawString(small, SCREEN_W / 2, 136, 2);
}

void SnakeApp::placeFood() {
  while (true) {
    P f{(int8_t)random(COLS), (int8_t)random(ROWS)};
    bool onSnake = false;
    for (int i = 0; i < len; i++)
      if (body[i].x == f.x && body[i].y == f.y) { onSnake = true; break; }
    if (!onSnake) { food = f; break; }
  }
}

void SnakeApp::turn(int d) {
  if ((d + 2) % 4 != dir) nextDir = d;  // can't reverse into yourself
}

void SnakeApp::onTouch(const TouchEvent& e) {
  if (over) {
    if (e.type == TouchEvent::TAP) needsReset = true;
    return;
  }
  if (paused) {
    if (e.type == TouchEvent::TAP) needsRedraw = true;  // unpause on the next update
    return;
  }
  switch (e.type) {
    case TouchEvent::SWIPE_UP:    turn(0); break;
    case TouchEvent::SWIPE_RIGHT: turn(1); break;
    case TouchEvent::SWIPE_DOWN:  turn(2); break;
    case TouchEvent::SWIPE_LEFT:  turn(3); break;
    case TouchEvent::TAP: {
      // Tap to the side of the head to turn that way
      int hx = body[0].x * CELL + CELL / 2, hy = TOP + body[0].y * CELL + CELL / 2;
      if (dir == 0 || dir == 2) turn(e.x < hx ? 3 : 1);
      else turn(e.y < hy ? 0 : 2);
      break;
    }
    default: break;
  }
}

void SnakeApp::update(TFT_eSPI& tft, uint32_t now) {
  g = &tft;
  if (needsReset) { needsReset = false; reset(); return; }
  if (needsRedraw) {
    needsRedraw = false;
    paused = false;
    drawBoard();
    lastStep = now;
    return;
  }
  if (over || paused) return;
  int speed = max(70, 160 - score * 4);
  if (now - lastStep < (uint32_t)speed) return;
  lastStep = now;
  step();
}

void SnakeApp::step() {
  static const int8_t DX[] = {0, 1, 0, -1}, DY[] = {-1, 0, 1, 0};
  dir = nextDir;
  P head{(int8_t)(body[0].x + DX[dir]), (int8_t)(body[0].y + DY[dir])};

  bool hitWall = head.x < 0 || head.x >= COLS || head.y < 0 || head.y >= ROWS;
  bool eat = head.x == food.x && head.y == food.y;
  bool hitSelf = false;
  int checkTo = eat ? len : len - 1;  // the tail moves away unless we grow
  for (int i = 0; i < checkTo && !hitWall; i++)
    if (body[i].x == head.x && body[i].y == head.y) { hitSelf = true; break; }

  if (hitWall || hitSelf) {
    over = true;
    if (Scores::submit("snake", score)) { best = score; Mood::event(Mood::HIGH_SCORE); }
    else Mood::event(Mood::GAME_LOST);
    drawScore();
    banner("Game over!", "tap to play again");
    return;
  }

  P tail = body[len - 1];
  if (eat && len < MAX_LEN) len++;
  for (int i = len - 1; i > 0; i--) body[i] = body[i - 1];
  body[0] = head;

  if (!eat) cell(tail, COL_BG);
  cell(body[1], SNAKE_BODY);
  cell(head, SNAKE_HEAD);

  if (eat) {
    score++;
    if (score > best) best = score;
    placeFood();
    drawFood();
    drawScore();
  }
}

void SnakeApp::drawIcon(TFT_eSPI& tft, int cx, int cy) {
  const int s = 6;
  const int8_t path[][2] = {{-2, 1}, {-1, 1}, {0, 1}, {0, 0}, {1, 0}, {1, -1}};
  for (int i = 0; i < 6; i++)
    tft.fillRect(cx - 3 + path[i][0] * s, cy - 3 + path[i][1] * s, s - 1, s - 1, i == 5 ? COL_EYE : 0x07E0);
  tft.fillCircle(cx + 13, cy + 9, 3, COL_PINK);
}
