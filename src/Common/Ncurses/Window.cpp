//
// Created by ab-flies on 16/11/24.
//

#include "Window.h"

#include <Common.h>
#include <stdexcept>
#include <fmt/format.h>

using namespace std;

Window::Window(const int height, const int width, const int start_y, const int start_x, const int margin_y,
               const int margin_x) {
    this->width = width;
    this->height = height;
    box = newwin(height, width, start_y, start_x);
    window = newwin(height - margin_y * 2, width - margin_x * 2, start_y + margin_y, start_x + margin_x);
}

Window::Window(Window &&other) noexcept {
    width = other.width;
    height = other.height;
    window = other.window;
    box = other.box;
    borders = other.borders;
    other.window = nullptr;
    other.box = nullptr;
}

Window &Window::operator=(Window &&other) noexcept {
    if (this != &other) { return *this; }

    if (box) { delwin(box); }
    if (window) { delwin(window); }

    window = other.window;
    box = other.box;
    borders = other.borders;
    other.window = nullptr;
    other.box = nullptr;

    return *this;
}

Window::~Window() {
    delwin(window);
    delwin(box);
}

void Window::unsetBorders() {
    for (unsigned &border: borders) {
        border = ' ';
    }
}


void Window::setBorder(BORDER border, const int character) {
    borders[static_cast<int>(border)] = character;
}

void Window::addString(const string &str, const int attribute) const {
    if (attribute != 0) { wattron(window, attribute); }
    waddstr(window, str.c_str());
    if (attribute != 0) { wattroff(window, attribute); }
}

void Window::addLine(const string &str, const int attribute) const {
    addString(str + "\n", attribute);
}

void Window::addMenu(const Menu &menu) const {
    menu.print(this);
}

void Window::addTables(const initializer_list<initializer_list<Table *> > tables, const bool center_y,
                       const bool center_x) const {
    size_t start_y;

    if (center_y) {
        size_t height = this->tableYMargin * (tables.size() - 1);
        for (const auto &tableRow: tables) {
            size_t rowMaxHeight = 0;
            for (const auto &table: tableRow) {
                rowMaxHeight = max(height, table->getHeight());
            }
            height += rowMaxHeight;
        }

        start_y = (this->height - height) / 2;
    } else {
        start_y = getcury(this->window);
    }

    for (const auto &tableRow: tables) {
        size_t start_x;

        if (center_x) {
            size_t length = tableRow.size() - 1;

            for (const auto &table: tableRow) {
                length += table->getLength();
            }

            length += this->tableXMargin * (tableRow.size() - 1);

            start_x = (this->width - length) / 2;
        } else {
            start_x = getcurx(this->window);
        }

        for (const auto &table: tableRow) {
            table->print(window, static_cast<int>(start_y), static_cast<int>(start_x));
            start_x += table->getLength() + this->tableXMargin;
        }

        start_y = getcury(this->window) + this->tableYMargin + 1;
    }
}


void Window::showBox() const {
    if (box) {
        wborder(box, borders[0], borders[1], borders[2], borders[3], borders[4], borders[5], borders[6], borders[7]);
        wrefresh(box);
    }
}

void Window::show() const {
    if (window) { wrefresh(window); }
}

Menu::Menu(string title, vector<string> options)
    : title(move(title)), options(move(options)) {
}

void Menu::hoverNext() {
    this->deselect();
    this->hovered = (this->hovered + 1) % this->options.size();
}

void Menu::hoverPrevious() {
    this->deselect();
    const size_t size = this->options.size();
    this->hovered = (this->hovered + size - 1) % size;
}

void Menu::print(const Window *window) const {
    window->clear();
    if (!this->title.empty()) {
        window->addLine(this->title, A_BOLD);
    }

    for (size_t i = 0; i < this->options.size(); i++) {
        string prefix = this->prefix;
        for (char &c: prefix) {
            if (c == '$') {
                c = (i == this->hovered ? this->hoverIndicator : ' ');
            } else if (c == '#') {
                c = (i == this->hovered && this->selected ? this->selectedIndicator : ' ');
            }
        }

        window->addLine(prefix + this->options[i]);
    }
}

Table::Table(const size_t colCount, vector<size_t> colWidths, string title) {
    if (colWidths.size() < colCount) {
        throw invalid_argument("Column widths must be at least the same as the column count");
    }

    this->title = move(title);
    this->colCount = colCount;
    this->hasHeaders = false;
    this->hasTitle = !this->title.empty();
    this->colWidths = move(colWidths);
    this->colExtraWidths = vector<int>(colCount, 0);
    this->rows = {};
    this->headers = {};

    this->length = 0; // To avoid compiler warning
    calculateLength();
}

Table::Table(vector<string> headers, string title) {
    this->title = move(title);
    this->hasTitle = !this->title.empty();
    this->hasHeaders = true;
    this->colCount = headers.size();
    this->headers = move(headers);
    this->rows = {};
    this->colExtraWidths = vector<int>(colCount, 0);

    this->colWidths = vector<size_t>(colCount, 0);
    for (size_t i = 0; i < this->colCount; i++) {
        this->colWidths[i] = this->headers[i].length() + 2;
    }

    this->length = 0; // To avoid compiler warning
    calculateLength();
}

void Table::clear() {
    this->rows.clear();
}

void Table::addRow(const vector<string> &row) {
    this->rows.emplace_back(colCount);

    size_t i;
    for (i = 0; i < colCount && i < row.size(); i++) {
        this->rows.back()[i] = row[i];
    }

    for (; i < colCount; i++) {
        this->rows.back()[i] = "";
    }
}

void Table::setRow(const size_t row, const size_t col, const string &value) {
    if (row >= rows.size() || col >= colCount) {
        return;
    }

    rows[row][col] = value;
}

void Table::calculateLength() {
    this->length = colCount + 1;
    for (const auto &n: colWidths) {
        this->length += n;
    }
}

void Table::widenColumn(const size_t col, const int width) {
    if (col >= colCount) {
        return;
    }

    this->colExtraWidths[col] = width;
    calculateLength();
}

template<typename T>
void Table::printLine(WINDOW *window, const unsigned left_corner, const T &fill, const unsigned col,
                      const unsigned right_corner) const {
    waddch(window, left_corner);
    for (size_t i = 0; i < this->colWidths.size(); i++) {
        if (i != 0) {
            waddch(window, col);
        }

        size_t colWidth = this->colWidths[i] + this->colExtraWidths[i];

        if constexpr (is_same_v<T, vector<string> >) {
            string str = fill[i];
            str = fmt::format("{:^{}}", str, colWidth); // Center the string
            waddstr(window, str.c_str());
        } else {
            // unsigned int or some char type
            for (size_t j = 0; j < colWidth; j++) {
                waddch(window, fill);
            }
        }
    }
    waddch(window, right_corner);
}

void Table::print(WINDOW *window, const int start_y, const int start_x) const {
    int line = 0;
#define nextLine() wmove(window, start_y + line++, start_x)

    // Title
    if (hasTitle) {
        nextLine();
        this->printLine(window, ACS_ULCORNER, ACS_HLINE, ACS_HLINE, ACS_URCORNER);

        nextLine();
        waddch(window, ACS_VLINE);
        wattron(window, A_BOLD);
        waddstr(window, fmt::format("{:^{}}", title, length - 2).c_str());
        wattroff(window, A_BOLD);
        waddch(window, ACS_VLINE);
    }

    // Headers
    nextLine();
    this->printLine(window, this->hasTitle ? ACS_LTEE : ACS_ULCORNER, ACS_HLINE, ACS_TTEE,
                    this->hasTitle ? ACS_RTEE : ACS_URCORNER);

    if (hasHeaders) {
        nextLine();
        this->printLine(window, ACS_VLINE, headers, ACS_VLINE, ACS_VLINE);
        nextLine();
        this->printLine(window, ACS_LTEE, ACS_HLINE, ACS_PLUS, ACS_RTEE);
    }

    // Rows
    for (const auto &row: rows) {
        if (this->foldEnabled) {
            vector<vector<string>> rowFolded;
            size_t maxLines = 0; // Maximum number of lines in a row

            // Fold the row
            for (size_t i = 0; i < colCount; i++) {
                rowFolded.push_back(splitLines(row[i], colWidths[i]));
                maxLines = max(maxLines, rowFolded.back().size());
            }

            // Balance the number of lines in each column
            for (auto& col: rowFolded) {
                while (col.size() < maxLines) {
                    col.emplace_back("");
                }
            }

            // Print the folded row
            for (size_t i = 0; i < rowFolded.front().size(); i++) {
                nextLine();
                vector<string> rowFoldedLine(colCount);
                for (size_t j = 0; j < colCount; j++) {
                    rowFoldedLine[j] = rowFolded[j][i];
                }
                this->printLine(window, ACS_VLINE, rowFoldedLine, ACS_VLINE, ACS_VLINE);
            }
        } else {
            nextLine();
            this->printLine(window, ACS_VLINE, row, ACS_VLINE, ACS_VLINE);
        }

        nextLine();
        if (hasHeaders) {
            this->printLine(window, ACS_VLINE, ' ', ACS_VLINE, ACS_VLINE);
        } else {
            this->printLine(window, ACS_LTEE, ACS_HLINE, ACS_PLUS, ACS_RTEE);
        }
    }

    // Closing line
    line--;
    nextLine();
    this->printLine(window, ACS_LLCORNER, ACS_HLINE, ACS_BTEE, ACS_LRCORNER);
}