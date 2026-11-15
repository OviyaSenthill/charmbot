#include "apps/SnakeApp.h"

static const uint16_t SNAKE_BODY = 0x07E0;
static const uint16_t SNAKE_HEAD = 0x3F3E;
static const uint16_t FOOD_COL   = 0xFA6D;
static const uint16_t BORDER     = 0x2124;

void SnakeApp::enter(TFT_eSPI& tft) {
  g = &tft;
  needsReset = false;
  reset();
}

void SnakeApp::reset() {
  g->fillScreen(COL_BG);
  g->drawRect(0, TOP - 1, SCREEN_W, ROWS * CELL + 2, BORDER);
  len = 4;
  for (int i = 0; i < len; i++) body[i] = {(int8_t)(8 - i), (int8_t)(ROWS / 2)};
  dir = nextDir = 1;
  score = 0;
  over = false;
  for (int i = 0; i < len; i++) cell(body[i], i == 0 ? SNAKE_HEAD : SNAKE_BODY);
  placeFood();
  drawScore();
  lastStep = millis();
}

void SnakeApp::cell(P p, uint16_t c) {
  g->fillRoundRect(p.x * CELL + 1, TOP + p.y * CELL + 1, CELL - 2, CELL - 2, 4, c);
}

void SnakeApp::drawScore() {
  g->fillRect(0, 0, SCREEN_W, TOP - 1, COL_BG);
  g->setTextDatum(TL_DATUM);
  g->setTextColor(COL_FG, COL_BG);
  g->drawString("Score " + String(score), 4, 0, 2);
  g->setTextDatum(TR_DATUM);
  g->setTextColor(COL_DIM, COL_BG);
  g->drawString("Best " + String(best) + "   hold = exit", SCREEN_W - 4, 0, 2);
}

void SnakeApp::placeFood() {
  while (true) {
    P f{(int8_t)random(COLS), (int8_t)random(ROWS)};
    bool onSnake = false;
    for (int i = 0; i < len; i++)
      if (body[i].x == f.x && body[i].y == f.y) { onSnake = true; break; }
    if (!onSnake) { food = f; break; }
  }
  g->fillCircle(food.x * CELL + CELL / 2, TOP + food.y * CELL + CELL / 2, CELL / 2 - 2, FOOD_COL);
}

void SnakeApp::turn(int d) {
  if ((d + 2) % 4 != dir) nextDir = d;  // can't reverse into yourself
}

void SnakeApp::onTouch(const TouchEvent& e) {
  if (over) {
    if (e.type == TouchEvent::TAP) needsReset = true;
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
  if (over) return;
  uint32_t speed = max(70, 160 - score * 4);
  if (now - lastStep < speed) return;
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
    if (score > best) best = score;
    g->setTextDatum(MC_DATUM);
    g->setTextColor(COL_FG, COL_BG);
    g->drawString(" Game over! ", SCREEN_W / 2, SCREEN_H / 2 - 12, 4);
    g->setTextColor(COL_DIM, COL_BG);
    g->drawString(" tap to play again ", SCREEN_W / 2, SCREEN_H / 2 + 16, 2);
    drawScore();
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
    drawScore();
    placeFood();
  }
}
