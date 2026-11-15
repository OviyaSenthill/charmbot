#include "apps/WifiApp.h"
#include "core/Net.h"
#include <WiFi.h>

// ---------- layout ----------
static const int LIST_Y = 50, ROW_H = 31;
static const int RESCAN_X = 232, RESCAN_Y = 8, RESCAN_W = 80, RESCAN_H = 30;
static const int KB_TOP = 58, KEY_H = 34, KEY_GAP = 2;

// Key kinds
enum { K_CHAR, K_SHIFT, K_BKSP, K_LAYER, K_SPACE, K_CANCEL, K_JOIN };

static const char* KB_ROWS[3][4] = {
  {"1234567890", "qwertyuiop", "asdfghjkl", "zxcvbnm"},
  {"1234567890", "QWERTYUIOP", "ASDFGHJKL", "ZXCVBNM"},
  {"1234567890", "!@#$%^&*()", "-_=+[]{}<>", ".,?/:;'\"~`"},
};

// Shorten text with "..." so it fits in maxW pixels
static String fitText(TFT_eSPI& tft, String s, int maxW, int font) {
  if (tft.textWidth(s, font) <= maxW) return s;
  while (s.length() > 1 && tft.textWidth(s + "...", font) > maxW) s.remove(s.length() - 1);
  return s + "...";
}

static void dotArc(TFT_eSPI& g, int cx, int cy, int r, int a0, int a1, int w, uint16_t c) {
  for (int a = a0; a <= a1; a += 10) {
    float rad = a * DEG_TO_RAD;
    g.fillCircle(cx + r * cosf(rad), cy + r * sinf(rad), w, c);
  }
}

// ---------- lifecycle ----------
void WifiApp::enter(TFT_eSPI& tft) {
  g = &tft;
  Net::setPaused(true);  // stop background reconnects while we scan / connect
  hasTap = false;
  pendingScroll = 0;
  startScan();
}

void WifiApp::exit() {
  if (state == SCANNING) WiFi.scanDelete();
  if (state == CONNECTING) Net::restoreSaved();  // abandoned mid-connect: go back to the saved network
  Net::setPaused(false);
  state = SCANNING;
}

void WifiApp::resume(TFT_eSPI& tft) {
  g = &tft;
  redrawAll();  // keep whatever you were typing
}

void WifiApp::onTouch(const TouchEvent& e) {
  if (e.type == TouchEvent::TAP) {
    hasTap = true;
    tapX = e.x;
    tapY = e.y;
  } else if (e.type == TouchEvent::SWIPE_UP) {
    pendingScroll += 3;
  } else if (e.type == TouchEvent::SWIPE_DOWN) {
    pendingScroll -= 3;
  }
}

// ---------- scanning ----------
void WifiApp::startScan() {
  scanRetries = 0;
  state = SCANNING;
  apCount = 0;
  scroll = 0;
  if (!Net::connected()) WiFi.disconnect();  // can't scan during a connection attempt
  WiFi.scanDelete();
  WiFi.scanNetworks(true);  // async: poll WiFi.scanComplete()
  stateAt = millis();
  redrawAll();
}

void WifiApp::collectScan(int n) {
  apCount = 0;
  for (int i = 0; i < n; i++) {
    String s = WiFi.SSID(i);
    if (s.isEmpty()) continue;  // hidden network
    int rssi = WiFi.RSSI(i);
    bool locked = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;

    int found = -1;  // the same name can appear on several access points
    for (int k = 0; k < apCount; k++)
      if (aps[k].ssid == s) { found = k; break; }
    if (found >= 0) {
      if (rssi > aps[found].rssi) aps[found].rssi = rssi;
      continue;
    }
    if (apCount < MAX_APS) aps[apCount++] = {s, rssi, locked};
  }
  WiFi.scanDelete();

  // Always list the network we're connected to, even if the scan missed it
  if (Net::connected() && apCount < MAX_APS) {
    String cur = WiFi.SSID();
    bool listed = false;
    for (int k = 0; k < apCount; k++)
      if (aps[k].ssid == cur) { listed = true; break; }
    if (!listed && cur.length()) aps[apCount++] = {cur, WiFi.RSSI(), true};
  }

  // Strongest first
  for (int i = 1; i < apCount; i++)
    for (int k = i; k > 0 && aps[k].rssi > aps[k - 1].rssi; k--) {
      AP t = aps[k]; aps[k] = aps[k - 1]; aps[k - 1] = t;
    }
}

// ---------- connecting ----------
void WifiApp::choose(int idx) {
  ssid = aps[idx].ssid;
  if (!aps[idx].locked) {
    pass = "";
    connect();
  } else if (ssid == Net::savedSsid() && Net::savedPassword().length()) {
    pass = Net::savedPassword();  // already know this one
    connect();
  } else {
    pass = "";
    layer = 0;
    shiftOnce = false;
    state = KEYBOARD;
    buildKeys();
    redrawAll();
  }
}

void WifiApp::connect() {
  state = CONNECTING;
  stateAt = millis();
  anim = 0;
  Net::connectTo(ssid, pass);
  redrawAll();
}

// ---------- update ----------
void WifiApp::update(TFT_eSPI& tft, uint32_t now) {
  g = &tft;

  if (hasTap) {
    hasTap = false;
    handleTap(tapX, tapY);
  }

  if (pendingScroll && state == LIST) {
    int maxScroll = max(0, apCount - ROWS_VISIBLE);
    int s = constrain(scroll + pendingScroll, 0, maxScroll);
    pendingScroll = 0;
    if (s != scroll) { scroll = s; drawList(); }
  }
  pendingScroll = 0;

  if (flashKey >= 0 && now > flashUntil) {
    if (state == KEYBOARD && flashKey < keyCount) drawKey(flashKey, false);
    flashKey = -1;
  }

  switch (state) {
    case SCANNING: {
      int n = WiFi.scanComplete();
      if (n >= 0) {
        collectScan(n);
        state = LIST;
        redrawAll();
      } else if (n == WIFI_SCAN_FAILED || now - stateAt > 12000) {
        // Quick (background) scans can fail while WiFi is busy. Retry, then do a slower, reliable scan.
        if (scanRetries < 2) {
          scanRetries++;
          WiFi.scanDelete();
          WiFi.scanNetworks(true);
          stateAt = now;
        } else {
          Serial.println("[wifi] quick scan failed, doing a full scan");
          int m = WiFi.scanNetworks(false);  // blocks ~2-4 s ("Scanning..." is already on screen)
          collectScan(m > 0 ? m : 0);
          state = LIST;
          redrawAll();
        }
      } else if (now - lastAnim > 300) {
        lastAnim = now;
        anim++;
        drawScanning();
      }
      break;
    }
    case CONNECTING: {
      uint32_t t = now - stateAt;
      wl_status_t s = WiFi.status();
      if (s == WL_CONNECTED) {
        Net::save();
        resultOk = true;
        resultMsg = "IP " + WiFi.localIP().toString();
        state = RESULT;
        stateAt = now;
        redrawAll();
      } else if (t > 15000 || (s == WL_CONNECT_FAILED && t > 3000) || (s == WL_NO_SSID_AVAIL && t > 6000)) {
        resultOk = false;
        resultMsg = s == WL_NO_SSID_AVAIL ? "Network not in range" : "Wrong password?";
        Net::restoreSaved();
        state = RESULT;
        stateAt = now;
        redrawAll();
      } else if (now - lastAnim > 300) {
        lastAnim = now;
        anim++;
        drawConnecting();
      }
      break;
    }
    case RESULT:
      if (resultOk && now - stateAt > 2000) {  // success: back to the list
        state = LIST;
        redrawAll();
      }
      break;
    default:
      break;
  }
}

void WifiApp::handleTap(int x, int y) {
  switch (state) {
    case LIST:
    case SCANNING:
      if (x >= RESCAN_X - 6 && y <= RESCAN_Y + RESCAN_H + 6) {
        startScan();
        return;
      }
      if (state == LIST && y >= LIST_Y) {
        int idx = scroll + (y - LIST_Y) / ROW_H;
        if (idx >= 0 && idx < apCount) choose(idx);
      }
      break;

    case KEYBOARD:
      for (int i = 0; i < keyCount; i++) {
        const Key& k = keys[i];
        if (x >= k.x && x < k.x + k.w && y >= k.y - 1 && y < k.y + k.h + 1) {
          pressKey(i);
          return;
        }
      }
      break;

    case RESULT:
      if (!resultOk) {  // try again
        if (pass.length() || Net::savedSsid() != ssid) {
          state = KEYBOARD;
          buildKeys();
        } else {
          state = LIST;
        }
        redrawAll();
      }
      break;

    default:
      break;
  }
}

// ---------- keyboard ----------
void WifiApp::buildKeys() {
  keyCount = 0;
  auto add = [&](int x, int y, int w, char c, uint8_t kind) {
    if (keyCount < MAX_KEYS) keys[keyCount++] = {(int16_t)x, (int16_t)y, (int16_t)w, (int16_t)KEY_H, c, kind};
  };

  for (int r = 0; r < 4; r++) {
    const char* row = KB_ROWS[layer][r];
    int n = strlen(row);
    int y = KB_TOP + r * (KEY_H + KEY_GAP);
    int left = 2, right = SCREEN_W - 2;

    if (r == 3) {  // shift on the left (not on the symbols page), backspace on the right
      if (layer != 2) { add(2, y, 44, 0, K_SHIFT); left = 48; }
      add(SCREEN_W - 46, y, 44, 0, K_BKSP);
      right = SCREEN_W - 48;
    }
    int pitch = min(32, (right - left) / n);
    int x0 = left + ((right - left) - pitch * n) / 2;
    for (int i = 0; i < n; i++) add(x0 + i * pitch, y, pitch - KEY_GAP, row[i], K_CHAR);
  }

  int y = KB_TOP + 4 * (KEY_H + KEY_GAP);
  add(2, y, 56, 0, K_LAYER);
  add(61, y, 120, ' ', K_SPACE);
  add(184, y, 60, 0, K_CANCEL);
  add(247, y, 71, 0, K_JOIN);
}

void WifiApp::drawKey(int i, bool pressed) {
  const Key& k = keys[i];
  uint16_t bg = COL_PANEL, fg = COL_FG;
  const char* label = nullptr;
  char ch[2] = {k.c, 0};

  switch (k.kind) {
    case K_CHAR:   label = ch; break;
    case K_SHIFT:  label = "Aa"; bg = shiftOnce ? COL_EYE : 0x2124; fg = shiftOnce ? COL_VISOR : COL_FG; break;
    case K_BKSP:   label = "DEL"; bg = 0x2124; break;
    case K_LAYER:  label = layer == 2 ? "abc" : "#+="; bg = 0x2124; break;
    case K_SPACE:  label = "space"; break;
    case K_CANCEL: label = "Back"; bg = 0x2124; break;
    case K_JOIN:   label = "Join"; bg = COL_EYE; fg = COL_VISOR; break;
  }
  if (pressed) { bg = COL_FG; fg = COL_VISOR; }

  g->fillRoundRect(k.x, k.y, k.w, k.h, 5, bg);
  g->setTextDatum(MC_DATUM);
  g->setTextColor(fg);
  g->drawString(label, k.x + k.w / 2, k.y + k.h / 2 + 1, 2);
}

void WifiApp::drawField() {
  g->fillRect(0, 0, SCREEN_W, KB_TOP - 2, COL_BG);
  g->setTextDatum(TL_DATUM);
  g->setTextColor(COL_DIM);
  g->drawString(fitText(*g, "Password for " + ssid, SCREEN_W - 12, 2), 6, 3, 2);

  g->fillRoundRect(4, 22, SCREEN_W - 8, 32, 6, COL_PANEL);
  // Show the end of the password if it's too long to fit
  String shown = pass;
  while (shown.length() && g->textWidth(shown, 4) > SCREEN_W - 30) shown.remove(0, 1);
  g->setTextColor(COL_FG);
  g->drawString(shown, 12, 25, 4);
  int cx = 12 + g->textWidth(shown, 4) + 2;
  g->fillRect(cx, 27, 2, 22, COL_EYE);  // cursor
}

void WifiApp::drawKeyboard() {
  g->fillRect(0, KB_TOP - 2, SCREEN_W, SCREEN_H - KB_TOP + 2, COL_BG);
  for (int i = 0; i < keyCount; i++) drawKey(i, false);
}

void WifiApp::pressKey(int i) {
  const Key& k = keys[i];
  bool rebuild = false;

  switch (k.kind) {
    case K_CHAR:
    case K_SPACE:
      if (pass.length() < 63) pass += k.c;
      if (shiftOnce) { shiftOnce = false; layer = 0; rebuild = true; }
      break;
    case K_BKSP:
      if (pass.length()) pass.remove(pass.length() - 1);
      break;
    case K_SHIFT:
      shiftOnce = !shiftOnce;
      layer = shiftOnce ? 1 : 0;
      rebuild = true;
      break;
    case K_LAYER:
      layer = layer == 2 ? 0 : 2;
      shiftOnce = false;
      rebuild = true;
      break;
    case K_CANCEL:
      state = LIST;
      redrawAll();
      return;
    case K_JOIN:
      connect();
      return;
  }

  drawField();
  if (rebuild) {
    buildKeys();
    drawKeyboard();
  } else {
    drawKey(i, true);  // flash the key you hit
    flashKey = i;
    flashUntil = millis() + 90;
  }
}

// ---------- screens ----------
void WifiApp::redrawAll() {
  g->fillScreen(COL_BG);
  switch (state) {
    case SCANNING:   drawHeader(); drawScanning(); break;
    case LIST:       drawHeader(); drawList(); break;
    case KEYBOARD:   drawField(); drawKeyboard(); break;
    case CONNECTING: drawConnecting(); break;
    case RESULT:     drawResult(); break;
  }
}

void WifiApp::drawHeader() {
  g->fillRect(0, 0, SCREEN_W, LIST_Y - 2, COL_BG);
  g->setTextDatum(TL_DATUM);
  g->setTextColor(COL_FG);
  g->drawString("WiFi", 8, 4, 4);

  if (Net::connected()) {
    g->setTextColor(COL_GOOD);
    g->drawString(fitText(*g, "Connected: " + Net::currentSsid(), 216, 2), 8, 31, 2);
  } else {
    g->setTextColor(COL_WARN);
    g->drawString("Not connected - pick a network", 8, 31, 2);
  }

  g->fillRoundRect(RESCAN_X, RESCAN_Y, RESCAN_W, RESCAN_H, 8, COL_PANEL);
  g->setTextDatum(MC_DATUM);
  g->setTextColor(COL_FG);
  g->drawString("Rescan", RESCAN_X + RESCAN_W / 2, RESCAN_Y + RESCAN_H / 2 + 1, 2);
}

void WifiApp::drawScanning() {
  g->fillRect(0, LIST_Y, SCREEN_W, SCREEN_H - LIST_Y, COL_BG);
  g->setTextDatum(MC_DATUM);
  g->setTextColor(COL_DIM);
  String dots = "";
  for (int i = 0; i < anim % 4; i++) dots += ".";
  g->drawString("Scanning" + dots + "   ", SCREEN_W / 2, 140, 4);
}

void WifiApp::drawList() {
  g->fillRect(0, LIST_Y, SCREEN_W, SCREEN_H - LIST_Y, COL_BG);

  if (apCount == 0) {
    g->setTextDatum(MC_DATUM);
    g->setTextColor(COL_FG);
    g->drawString("No networks found", SCREEN_W / 2, 120, 4);
    g->setTextColor(COL_DIM);
    g->drawString("The board only sees 2.4 GHz WiFi.", SCREEN_W / 2, 152, 2);
    g->drawString("Tip: phone hotspot + Maximize Compatibility", SCREEN_W / 2, 172, 2);
    return;
  }

  String current = Net::connected() ? Net::currentSsid() : String("");
  for (int i = 0; i < ROWS_VISIBLE; i++) {
    int idx = scroll + i;
    if (idx >= apCount) break;
    const AP& ap = aps[idx];
    int y = LIST_Y + i * ROW_H;
    bool isCurrent = ap.ssid == current;

    g->fillRoundRect(4, y, SCREEN_W - 14, ROW_H - 3, 6, COL_PANEL);
    if (isCurrent) g->drawRoundRect(4, y, SCREEN_W - 14, ROW_H - 3, 6, COL_GOOD);

    g->setTextDatum(ML_DATUM);
    g->setTextColor(isCurrent ? COL_GOOD : COL_FG);
    g->drawString(fitText(*g, ap.ssid, 220, 2), 12, y + ROW_H / 2 - 1, 2);

    // Lock icon
    if (ap.locked) {
      int lx = 246, ly = y + 9;
      g->drawRoundRect(lx + 1, ly - 4, 7, 8, 3, COL_DIM);
      g->fillRect(lx, ly, 9, 8, COL_DIM);
    }
    // Signal bars
    int bars = ap.rssi > -55 ? 4 : ap.rssi > -65 ? 3 : ap.rssi > -75 ? 2 : 1;
    for (int b = 0; b < 4; b++) {
      int bh = 4 + b * 4;
      g->fillRect(264 + b * 7, y + ROW_H - 9 - bh, 5, bh, b < bars ? COL_EYE : 0x39E7);
    }
  }

  // Scroll hints
  if (scroll > 0) g->fillTriangle(314, LIST_Y + 2, 310, LIST_Y + 10, 318, LIST_Y + 10, COL_DIM);
  if (scroll + ROWS_VISIBLE < apCount) g->fillTriangle(314, SCREEN_H - 4, 310, SCREEN_H - 12, 318, SCREEN_H - 12, COL_DIM);
}

void WifiApp::drawConnecting() {
  g->fillRect(0, 80, SCREEN_W, 90, COL_BG);
  g->setTextDatum(MC_DATUM);
  g->setTextColor(COL_FG);
  g->drawString(fitText(*g, "Connecting to " + ssid, SCREEN_W - 20, 4), SCREEN_W / 2, 105, 4);
  for (int i = 0; i < 3; i++) g->fillCircle(SCREEN_W / 2 - 20 + i * 20, 145, 5, i == anim % 3 ? COL_EYE : COL_PANEL);
}

void WifiApp::drawResult() {
  g->setTextDatum(MC_DATUM);
  if (resultOk) {
    g->fillCircle(SCREEN_W / 2, 80, 24, COL_GOOD);
    g->drawWideLine(SCREEN_W / 2 - 11, 80, SCREEN_W / 2 - 3, 89, 5, COL_BG, COL_GOOD);
    g->drawWideLine(SCREEN_W / 2 - 3, 89, SCREEN_W / 2 + 12, 71, 5, COL_BG, COL_GOOD);
    g->setTextColor(COL_FG);
    g->drawString("Connected!", SCREEN_W / 2, 130, 4);
    g->setTextColor(COL_DIM);
    g->drawString(fitText(*g, ssid + "  -  saved for next time", SCREEN_W - 20, 2), SCREEN_W / 2, 160, 2);
    g->drawString(resultMsg, SCREEN_W / 2, 180, 2);
  } else {
    g->fillCircle(SCREEN_W / 2, 80, 24, COL_PINK);
    g->drawWideLine(SCREEN_W / 2 - 9, 71, SCREEN_W / 2 + 9, 89, 5, COL_BG, COL_PINK);
    g->drawWideLine(SCREEN_W / 2 + 9, 71, SCREEN_W / 2 - 9, 89, 5, COL_BG, COL_PINK);
    g->setTextColor(COL_FG);
    g->drawString("Couldn't connect", SCREEN_W / 2, 130, 4);
    g->setTextColor(COL_DIM);
    g->drawString(resultMsg, SCREEN_W / 2, 160, 2);
    g->drawString("tap to try again", SCREEN_W / 2, 185, 2);
  }
}

// ---------- wheel icon ----------
void WifiApp::drawIcon(TFT_eSPI& tft, int cx, int cy) {
  uint16_t c = Net::connected() ? COL_EYE : COL_DIM;
  tft.fillCircle(cx, cy + 9, 3, c);
  dotArc(tft, cx, cy + 9, 9, 225, 315, 2, c);
  dotArc(tft, cx, cy + 9, 16, 225, 315, 2, c);
  dotArc(tft, cx, cy + 9, 23, 230, 310, 2, c);
}
