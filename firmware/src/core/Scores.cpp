#include "core/Scores.h"
#include <Preferences.h>

namespace Scores {

int best(const char* game) {
  Preferences prefs;
  if (!prefs.begin("scores", true)) return 0;  // nothing saved yet
  int v = prefs.getInt(game, 0);
  prefs.end();
  return v;
}

bool submit(const char* game, int score) {
  if (score <= best(game)) return false;
  Preferences prefs;
  prefs.begin("scores", false);
  prefs.putInt(game, score);
  prefs.end();
  return true;
}

}  // namespace Scores
