#include "settings.h"
#include <stdio.h>
#include <string.h>

static const char *diffName(AIDifficulty d) {
    switch (d) {
        case AI_EASY: return "easy";
        case AI_MEDIUM: return "medium";
        case AI_HARD: return "hard";
        case AI_EXPERT: return "expert";
        default: return "medium";
    }
}

static AIDifficulty parseDiff(const char *s) {
    if (strcmp(s, "easy") == 0) return AI_EASY;
    if (strcmp(s, "hard") == 0) return AI_HARD;
    if (strcmp(s, "expert") == 0) return AI_EXPERT;
    return AI_MEDIUM;
}

bool loadSettings(const char *filename, Settings *set) {
    FILE *f = fopen(filename, "r");
    if (!f) return false;
    char key[64];
    char value[192];
    while (fscanf(f, "%63[^=]=%191s\n", key, value) == 2) {
        if (strcmp(key, "mode") == 0) {
            set->mode = (strcmp(value, "human_vs_ai") == 0) ? MODE_HUMAN_VS_AI : MODE_HUMAN_VS_HUMAN;
        } else if (strcmp(key, "difficulty") == 0) {
            set->difficulty = parseDiff(value);
        } else if (strcmp(key, "theme") == 0) {
            set->theme = (strcmp(value, "alt") == 0) ? 1 : 0;
        } else if (strcmp(key, "flip") == 0) {
            set->flipBoard = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
        } else if (strcmp(key, "pgn_file") == 0) {
            strncpy(set->pgnFile, value, sizeof(set->pgnFile)-1);
            set->pgnFile[sizeof(set->pgnFile)-1] = '\0';
        }
    }
    fclose(f);
    return true;
}

bool saveSettings(const char *filename, const Settings *set) {
    FILE *f = fopen(filename, "w");
    if (!f) return false;
    fprintf(f, "mode=%s\n", set->mode == MODE_HUMAN_VS_AI ? "human_vs_ai" : "human_vs_human");
    fprintf(f, "difficulty=%s\n", diffName(set->difficulty));
    fprintf(f, "theme=%s\n", set->theme ? "alt" : "classic");
    fprintf(f, "flip=%s\n", set->flipBoard ? "true" : "false");
    fprintf(f, "pgn_file=%s\n", set->pgnFile);
    fclose(f);
    return true;
}
