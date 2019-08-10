#include "font_setting.h"

void SwitchFont(FontSetting *fontSetting) {
    if (fontSetting->clearFontEnabled) {
        fontSetting->clearFontEnabled = false;
        fontSetting->currentFont = &fontSetting->mainFont;
    } else {
        fontSetting->clearFontEnabled = true;
        fontSetting->currentFont = &fontSetting->clearFont;
    }
}
