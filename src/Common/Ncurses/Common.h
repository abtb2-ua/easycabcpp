//
// Created by ab-flies on 16/11/24.
//

#ifndef NCURSES_COMMON_H
#define NCURSES_COMMON_H

#include "Logs.h"

using namespace code_logs;

/// @note The color values start at 100 to avoid conflicts with terminal colors
enum class COLOR {
    RED = 100,
    GREEN,
    YELLOW,
    BLUE,
    MAGENTA,
    PURPLE,
    ORANGE
};

#define ctrl(x) ((x) & 0x1f)

void initCurses();
void endCurses(const string &message = "");

void initColorWrapper(COLOR _color, short r, short g, short b);
void startColors();
COLOR getCodeNcursesColor(const LogType &code);
#endif
