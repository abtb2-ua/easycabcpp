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

// using namespace std;

unique_ptr<Window> win_left;
unique_ptr<Window> win_top;
unique_ptr<Window> win_bottom;
unique_ptr<Window> win_tables;

void initWindows();

void ncursesGui(const void *_pipeReader) {
    // const auto pipe_to_gui = static_cast<int*>(_pipe_to_gui);
    // close(pipe_to_gui[1]);
    const auto pipeReader = static_cast<const PipeHandler::PipeReader*>(_pipeReader);

    initCurses();
    initWindows();

    Table table1(3, {10, 15, 20}, "Example Table 1");
    table1.addRow({"Row1 Col1", "Row1 Col2", "Row1 Col3"});
    table1.addRow({"Row2 Col1", "Row2 Col2", "Row2 Col3"});

    // Example Table 2
    Table table2({"Header1", "Header2", "Header3"}, "Example Table 2");
    table2.addRow({"Row1 Col1", "Row1 Col2", "Testing Col3 Col3Col3 Col3"});
    table2.addRow({"Row2 Col1", "Row2 Col2", "Row2 Col3"});
    table2.enableFold();

    win_tables->addTables({{&table1, &table2}, {&table2}}, true, true);
    win_tables->show();

    getch();
    // tables.showBox();
    //
    // left.addString("Left window");
    // top.addLine("Top window");
    // top.addLine("Top window");
    // top.addLine("Top window");
    // bottom.addString("Bottom window");
    //
    // // left.show();
    // top.show();
    // // bottom.show();
    //
    // getch();


    endwin();
    exit(0);
}

void initWindows() {
    const int max_x = getmaxx(stdscr);
    const int max_y = getmaxy(stdscr);

    win_left = make_unique<Window>(max_y, WIDTH, 0, 0, 2, 2);
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

    win_left->showBox();
}
