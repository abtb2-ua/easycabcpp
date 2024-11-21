//
// Created by ab-flies on 15/11/24.
//

#include <ncurses.h>
#include "GUI.h"

#include <memory>
#include <PipeHandler.h>

#include "Ncurses/Window.h"
#include "Ncurses/Common.h"
#include "PipeHandler.h"

#define WIDTH 30
#define HEIGHT 25

using namespace std;
using namespace code_logs;

unique_ptr<Window> win_menu;
unique_ptr<Window> win_top;
unique_ptr<Window> win_bottom;
unique_ptr<Window> win_tables;

bool showTableView = false;

void ncursesGui(PipeWriter &&pipeWriter, PipeReader &&pipeReader) {
    initCurses();
    initWindows();
    pipeReader.setTimeout(0);
    win_menu->setTimeout(0);

    printMenu();

    while (true) {
        listenPipe(pipeReader);

        const int ch = win_menu->getChar();

        if (ch == ERR) {
            continue;
        }

        if (ch == 'q' || ch == 'Q') {
            break;
        }



        handleUserInput(ch);

        printMenu();

        if (showTableView) {
            printTableView();
        } else {
            printLogs();
        }
    }

    getch();
    endwin();
    cout << "Exiting gui";
    exit(0);
}

void listenPipe(PipeReader &pipeReader) {
    if (pipeReader.readFromPipe() == READ_RETURN_CODE::READ_DATA) {
        const auto buffer = pipeReader.getBuffer();
    }

}


void initWindows() {
    const int max_x = getmaxx(stdscr);
    const int max_y = getmaxy(stdscr);

    win_menu = make_unique<Window>(max_y, WIDTH, 0, 0, 2, 2);
    win_top = make_unique<Window>(HEIGHT, max_x - WIDTH, 0, WIDTH, 1, 1);
    win_bottom = make_unique<Window>(max_y - HEIGHT, max_x - WIDTH + 1, HEIGHT, WIDTH - 1, 1, 2);
    win_tables = make_unique<Window>(max_y, max_x - WIDTH + 1, 0, WIDTH - 1, 1, 2);

    win_bottom->unsetBorders();
    win_tables->unsetBorders();

    win_bottom->setBorder(BORDER::TOP, ACS_HLINE);
    win_bottom->setBorder(BORDER::TOP_LEFT, ACS_LTEE);
    win_bottom->setBorder(BORDER::LEFT, ACS_VLINE);
    win_bottom->setBorder(BORDER::BOTTOM_LEFT, ACS_LRCORNER);

    win_tables->setBorder(BORDER::TOP_LEFT, ACS_URCORNER);
    win_tables->setBorder(BORDER::LEFT, ACS_VLINE);
    win_tables->setBorder(BORDER::BOTTOM_LEFT, ACS_LRCORNER);

    win_menu->showBox();
}


