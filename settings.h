#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdbool.h>
#include "chess.h"
#include "ai.h"

// Structure storing user preferences
typedef struct {
    GameMode mode;
    AIDifficulty difficulty;
    int theme;          // corresponds to UITheme enum
    bool flipBoard;
    char pgnFile[256];
} Settings;

bool loadSettings(const char *filename, Settings *settings);
bool saveSettings(const char *filename, const Settings *settings);

#endif /* SETTINGS_H */
