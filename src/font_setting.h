#ifndef FONT_SETTING_H
#define FONT_SETTING_H

#include "raylib.h"

typedef struct {
    Font mainFont;
    Font clearFont;
    bool clearFontEnabled;
    Font *currentFont;
} FontSetting;

void SwitchFont(FontSetting *fontSetting);

#endif
