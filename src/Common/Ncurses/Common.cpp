//
// Created by ab-flies on 16/11/24.
//

#include "Ncurses/Common.h"
#include <ncurses.h>

void initCurses() {
    initscr();
    curs_set(0);
    keypad(stdscr, true);
    noecho();
    raw();

    refresh();
}

void init_color_wrapper(COLOR color, short r, short g, short b) {
    r = static_cast<short>(r / 256 * 1000);
    g = static_cast<short>(g / 256 * 1000);
    b = static_cast<short>(b / 256 * 1000);
    init_color(static_cast<short>(color), r, g, b);
    init_pair(static_cast<short>(color), static_cast<short>(color), COLOR_BLACK);
}

void startColors() {
    start_color();

    init_color(COLOR_BLACK, 75, 75, 75);

    init_color_wrapper(COLOR::RED, 255, 105, 97);
    init_color_wrapper(COLOR::GREEN, 119, 221, 119);
    init_color_wrapper(COLOR::YELLOW, 253, 253, 150);
    init_color_wrapper(COLOR::BLUE, 132, 182, 244);
    init_color_wrapper(COLOR::MAGENTA, 253, 202, 225);
    init_color_wrapper(COLOR::PURPLE, 216, 132, 244);
    init_color_wrapper(COLOR::ORANGE, 255, 189, 102);
}


void endCurses() {
    endwin();
}