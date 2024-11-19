//
// Created by ab-flies on 16/11/24.
//

#ifndef COMMON_H
#define COMMON_H

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

void initCurses();

#endif //COMMON_H
