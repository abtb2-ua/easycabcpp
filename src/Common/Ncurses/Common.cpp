//
// Created by ab-flies on 16/11/24.
//

#include "Ncurses/Common.h"

#include <Common.h>
#include <ncurses.h>

#include "Window.h"

void initCurses() {
    initscr();
    keypad(stdscr, true);
    noecho();
    raw();
    curs_set(0);
    startColors();

    refresh();
}

void initColorWrapper(COLOR _color, short r, short g, short b) {
    const short color = static_cast<short>(_color);
    r = r * 1000 / 256;
    g = g * 1000 / 256;
    b = b * 1000 / 256;
    init_color(color, r, g, b);
    init_pair(color, color, COLOR_BLACK);
}

void startColors() {
    start_color();

    init_color(COLOR_BLACK, 75, 75, 75);

    initColorWrapper(COLOR::RED, 255, 105, 97);
    initColorWrapper(COLOR::GREEN, 119, 221, 119);
    initColorWrapper(COLOR::YELLOW, 253, 253, 150);
    initColorWrapper(COLOR::BLUE, 132, 182, 244);
    initColorWrapper(COLOR::MAGENTA, 253, 202, 225);
    initColorWrapper(COLOR::PURPLE, 216, 132, 244);
    initColorWrapper(COLOR::ORANGE, 255, 189, 102);
}


void endCurses(const string &message) {
    const int maxX = getmaxx(stdscr);
    const int maxY = getmaxy(stdscr);

    constexpr int width = 40;
    vector<string> content;
    const string prefix = "Error: ";
    if (!message.empty()) {
        content = splitLines(prefix + message, width - 2);
    }

    const int height = 5 + content.size();

    const Window popUp(height, width, (maxY - height) / 2, (maxX - width) / 2, 1, 1);
    popUp.disableScroll();

    for (size_t i = 0; i < content.size(); i++) {
        if (i == 0) {
            string centered = centerString(content[i], width - 3);
            const auto pos = centered.find(prefix) + prefix.length();
            popUp.addString(centered.substr(0, pos), COLOR_PAIR(COLOR::RED));
            popUp.addLine(centered.substr(pos));
        } else {
            popUp.addLine(centerString(content[i], width - 3));
        }
    }

    popUp.addLine(centerString("Exiting...", width - 3), COLOR_PAIR(COLOR::RED));
    popUp.addLine();
    popUp.addLine(centerString("(Press any key to exit)", width - 3), A_DIM);
    popUp.showBox();
    popUp.show();

    notimeout(stdscr, false);
    getch();
    endwin();
}

COLOR getCodeNcursesColor(const LogType &code) {
    if (holds_alternative<ERROR>(code)) {
        return COLOR::RED;
    }
    if (holds_alternative<WARNING>(code)) {
        return COLOR::ORANGE;
    }

    const auto message = get<MESSAGE>(code);
    if (message == MESSAGE::DEBUG) {
        return COLOR::MAGENTA;
    }

    return COLOR::GREEN;
}
