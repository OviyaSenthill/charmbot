#include "apps/Game2048App.h"
#include "core/Scores.h"
#include "core/Mood.h"

// Layout: 4x4 board on the left, score panel on the right
static const int TILE = 48, GAP = 6;
static const int BOARD = 4 * TILE + 5 * GAP;  // 222
static const int BX = 8, BY = (SCREEN_H - BOARD) / 2;
static const int PANEL_X = BX + BOARD + 8;     // 238
static const int PANEL_CX = (PANEL_X + SCREEN_W) / 2;
static const int NEW_Y = 186, NEW_H = 40;

// Tile colors: cool blues -> cyan -> warm -> pink -> purple -> white
static uint16_t tileColor(uint16_t v) {
  switch (v) {
    case 0:    return COL_PANEL;
    case 2:    return 0x2A4F;  // #2E4A7A
    case 4:    return 0x3B74;  // #3A6EA5
    case 8:    return 0x3F3E;  // #3FE6F5
    case 16:   return 0x3E94;  // #3FD0A0
    case 32:   return 0x8F09;  // #8BE04A
    case 64:   return 0xF687;  // #F5D33F
    case 128:  return 0xF507;  // #F5A33F
    case 256:  return 0xF3A7;  // #F5763F
    case 512:  return 0xFA6D;  // #FF4D6D
    case 1024: return 0xDA7F;  // #D94DFF
    case 2048: return 0xFFFF;
    default:   return 0xFFE0;
  }
}

void Game2048App::newGame() {
  if (score > 0) Scores::submit("2048", score);
  memset(grid, 0, sizeof(grid));
  score = 0;
  over = won = showWin = false;
  started = true;
  spawn();
  spawn();
  lastSpawnR = lastSpawnC = -1;
}

void Game2048App::spawn() {
  int empty[16][2], n = 0;
  for (int r = 0; r < 4; r++)
    for (int c = 0; c < 4; c++)
      if (!grid[r][c]) { empty[n][0] = r; empty[n][1] = c; n++; }
  if (!n) return;
  int k = random(n);
  grid[empty[k][0]][empty[k][1]] = random(10) == 0 ? 4 : 2;
  lastSpawnR = empty[k][0];
  lastSpawnC = empty[k][1];
}

// Slide + merge every row/column toward the edge in `dir`. Returns true if anything moved.
bool Game2048App::move(int dir) {
  bool moved = false;
  for (int line = 0; line < 4; line++) {
    int pos[4][2];  // cells of this line, starting at the edge we slide toward
    for (int k = 0; k < 4; k++) {
      int r, c;
      switch (dir) {
        case 0:  r = k;     c = line;  break;  // up
        case 2:  r = 3 - k; c = line;  break;  // down
        case 3:  r = line;  c = k;     break;  // left
        default: r = line;  c = 3 - k; break;  // right
      }
      pos[k][0] = r;
      pos[k][1] = c;
    }

    uint16_t vals[4];
    int n = 0;
    for (int k = 0; k < 4; k++) {
      uint16_t v = grid[pos[k][0]][pos[k][1]];
      if (v) vals[n++] = v;
    }

    uint16_t out[4] = {0, 0, 0, 0};
    int o = 0;
    for (int i = 0; i < n; i++) {
      if (i + 1 < n && vals[i] == vals[i + 1]) {
        out[o] = vals[i] * 2;
        score += out[o];
        if (out[o] == 2048 && !won) { won = true; showWin = true; }
        o++;
        i++;  // skip the tile we merged with
      } else {
        out[o++] = vals[i];
      }
    }

    for (int k = 0; k < 4; k++) {
      uint16_t& cell = grid[pos[k][0]][pos[k][1]];
      if (cell != out[k]) moved = true;
      cell = out[k];
    }
  }
  return moved;
}

bool Game2048App::canMove() const {
  for (int r = 0; r < 4; r++)
    for (int c = 0; c < 4; c++) {
      if (!grid[r][c]) return true;
      if (c < 3 && grid[r][c] == grid[r][c + 1]) return true;
      if (r < 3 && grid[r][c] == grid[r + 1][c]) return true;
    }
  return false;
}

// ---------- touch ----------
void Game2048App::onTouch(const TouchEvent& e) {
  if (e.type == TouchEvent::TAP) {
    bool onNew = e.x >= PANEL_X && e.y >= NEW_Y - 4;
    if (over || onNew) needsNewGame = true;
    else if (showWin) { showWin = false; needsRedraw = true; }  // keep playing after 2048
    return;
  }
  if (over || showWin) return;
  switch (e.type) {
    case TouchEvent::SWIPE_UP:    pendingMove = 0; break;
    case TouchEvent::SWIPE_RIGHT: pendingMove = 1; break;
    case TouchEvent::SWIPE_DOWN:  pendingMove = 2; break;
    case TouchEvent::SWIPE_LEFT:  pendingMove = 3; break;
    default: break;
  }
}

// ---------- loop ----------
void Game2048App::enter(TFT_eSPI& tft) {
  best = Scores::best("2048");
  if (!started) newGame();
  drawAll(tft);
}

void Game2048App::update(TFT_eSPI& tft, uint32_t now) {
  if (needsNewGame) {
    needsNewGame = false;
    newGame();
    drawAll(tft);
    return;
  }
  if (needsRedraw) {
    needsRedraw = false;
    drawAll(tft);
    return;
  }
  if (pendingMove < 0) return;

  int m = pendingMove;
  pendingMove = -1;
  if (!move(m)) return;  // nothing slid: ignore the swipe

  spawn();
  if (score > best) best = score;
  drawTiles(tft);
  drawPanel(tft);

  if (showWin) {
    drawBanner(tft, "2048!", "tap to keep going");
  } else if (!canMove()) {
    over = true;
    Mood::event(Scores::submit("2048", score) ? Mood::HIGH_SCORE : Mood::GAME_LOST);
    drawBanner(tft, "No moves left", "tap to play again");
  }
}

// ---------- drawing ----------
void Game2048App::drawAll(TFT_eSPI& tft) {
  tft.fillScreen(COL_BG);
  tft.fillRoundRect(BX, BY, BOARD, BOARD, 8, 0x10A4);
  drawTiles(tft);
  drawPanel(tft);
  if (over) drawBanner(tft, "No moves left", "tap to play again");
  else if (showWin) drawBanner(tft, "2048!", "tap to keep going");
}

void Game2048App::drawTiles(TFT_eSPI& tft) {
  for (int r = 0; r < 4; r++)
    for (int c = 0; c < 4; c++) drawTile(tft, r, c);
}

void Game2048App::drawTile(TFT_eSPI& tft, int r, int c) {
  int x = BX + GAP + c * (TILE + GAP);
  int y = BY + GAP + r * (TILE + GAP);
  uint16_t v = grid[r][c];
  tft.fillRoundRect(x, y, TILE, TILE, 6, tileColor(v));
  if (r == lastSpawnR && c == lastSpawnC && v) tft.drawRoundRect(x, y, TILE, TILE, 6, COL_FG);  // new tile outline
  if (!v) return;

  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(v <= 4 ? COL_FG : COL_VISOR);
  tft.drawNumber(v, x + TILE / 2, y + TILE / 2 + 1, v < 1000 ? 4 : 2);
}

void Game2048App::drawPanel(TFT_eSPI& tft) {
  tft.fillRect(PANEL_X, 0, SCREEN_W - PANEL_X, SCREEN_H, COL_BG);
  tft.setTextDatum(MC_DATUM);

  tft.setTextColor(COL_EYE);
  tft.drawString("2048", PANEL_CX, 26, 4);

  tft.setTextColor(COL_DIM);
  tft.drawString("SCORE", PANEL_CX, 64, 2);
  tft.setTextColor(COL_FG);
  tft.drawNumber(score, PANEL_CX, 86, score < 10000 ? 4 : 2);

  tft.setTextColor(COL_DIM);
  tft.drawString("BEST", PANEL_CX, 122, 2);
  tft.setTextColor(COL_FG);
  tft.drawNumber(best, PANEL_CX, 144, best < 10000 ? 4 : 2);

  tft.fillRoundRect(PANEL_X + 4, NEW_Y, SCREEN_W - PANEL_X - 10, NEW_H, 8, COL_PANEL);
  tft.drawRoundRect(PANEL_X + 4, NEW_Y, SCREEN_W - PANEL_X - 10, NEW_H, 8, COL_EYE);
  tft.setTextColor(COL_FG);
  tft.drawString("New", PANEL_CX - 3, NEW_Y + NEW_H / 2, 4);
}

void Game2048App::drawBanner(TFT_eSPI& tft, const char* big, const char* small) {
  int w = BOARD - 24, h = 70;
  int x = BX + 12, y = BY + (BOARD - h) / 2;
  tft.fillRoundRect(x, y, w, h, 10, COL_VISOR);
  tft.drawRoundRect(x, y, w, h, 10, COL_EYE);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(COL_FG);
  tft.drawString(big, x + w / 2, y + 24, 4);
  tft.setTextColor(COL_DIM);
  tft.drawString(small, x + w / 2, y + 52, 2);
}

void Game2048App::drawIcon(TFT_eSPI& tft, int cx, int cy) {
  tft.fillRoundRect(cx - 16, cy - 16, 32, 32, 6, 0xF687);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(COL_VISOR);
  tft.drawString("2048", cx, cy + 1, 2);
}
