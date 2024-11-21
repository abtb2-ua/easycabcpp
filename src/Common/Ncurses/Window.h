//
// Created by ab-flies on 16/11/24.
//

#ifndef WINDOW_H
#define WINDOW_H

#include <array>
#include <ncurses.h>
#include <queue>
#include <string>

using namespace std;

enum class BORDER {
    LEFT,
    RIGHT,
    TOP,
    BOTTOM,
    TOP_LEFT,
    TOP_RIGHT,
    BOTTOM_LEFT,
    BOTTOM_RIGHT,
};

class Table;
class Menu;
class Window;

/// @note Lacks some features for multiple windows applications.
/// Consider using other classes like LogWindow or MenuWindow for that matter
class Window {
    WINDOW *window;
    WINDOW *box;
    array<unsigned, 8> borders = {};
    int width, height;
    int tableXMargin = 1;
    int tableYMargin = 1;

public:
    // Canonical form
    Window() = delete;

    /// @note Margins include the border. That is, if margins are 0 (default), the content will override the border.
    Window(int height, int width, int start_y, int start_x, int margin_y = 0, int margin_x = 0);

    Window(const Window &) = delete;

    Window(Window &&other) noexcept;

    Window &operator=(const Window &) = delete;

    Window &operator=(Window &&other) noexcept;

    ~Window();

    // Methods
    void addString(const string &str, int attribute = 0) const;

    void addLine(const string &str, int attribute = 0) const;

    void addTables(initializer_list<initializer_list<Table *>> tables, bool center_y = false, bool center_x = true) const;

    void setTableXMargin(const int margin) { tableXMargin = max(margin, 0); }
    void setTableYMargin(const int margin) { tableYMargin = max(margin, 0); }

    void addMenu(const Menu &menu) const;

    void setTimeout(const int milliseconds) const { wtimeout(this->window, milliseconds); }

    [[nodiscard]] int getChar() const { return wgetch(window); }

    void show() const;

    /// @brief Show the box around the window
    /// @details This method does not refresh the window and overrides the window content.
    /// To avoid flickering, avoid calling this method on update loops.
    void showBox() const;

    void clear() const { werase(window); }

    void unsetBorders();

    void setBorder(BORDER border, int character);
};

class Menu {
    friend class Window;

    string title;
    vector<string> options;
    bool selected = false;
    size_t hovered = 0;

    /// @var prefix
    /// @brief Prefix to be added to the options
    /// @details To indicate where the hover and selected indicators should go use $ and # respectively
    string prefix = " $# ";
    char hoverIndicator = '>';
    char selectedIndicator = '>'; // Character to indicate the selected option

    void print(const Window *window) const;

public:
    Menu(): title(), options() {
    }

    Menu(string title, vector<string> options);

    void setPrefix(const string &prefix) { this->prefix = prefix; }
    void setHoverIndicator(const char hoverIndicator) { this->hoverIndicator = hoverIndicator; }
    void setSelectedIndicator(const char selectedIndicator) { this->selectedIndicator = selectedIndicator; }
    void setTitle(const string &title) { this->title = title; }
    void setOptions(const vector<string> &options) { this->options = options; }
    void addOption(const string &option) { options.push_back(option); }

    void hoverNext();

    void hoverPrevious();

    void select() { selected = true; }
    void deselect() { selected = false; }

    [[nodiscard]] size_t getHovered() const { return hovered; }
    [[nodiscard]] bool isSelected() const { return selected; }
};

class Table {
    friend class Window;

    string title;
    vector<string> headers;
    vector<size_t> colWidths;
    vector<int> colExtraWidths;
    vector<vector<string> > rows;
    bool hasTitle;
    bool hasHeaders;
    size_t colCount;
    size_t length; // Length of the table when printed
    bool foldEnabled = false;

    void calculateLength();

    template <typename T>
    void printLine(WINDOW *window, unsigned left_corner, const T &fill, unsigned col, unsigned right_corner) const;

    void print(WINDOW *window, int start_y, int start_x) const;

    [[nodiscard]] vector<vector<string>> foldRow(const vector<string> &row) const;
public:
    explicit Table(size_t colCount, vector<size_t> colWidths, string title = "");

    explicit Table(vector<string> headers, string title = "");

    void clear();

    /// @brief Adds a row to the table
    /// @details If the row has more columns than the table, the extra columns will be ignored.
    /// If the row has fewer columns than the table, the missing columns will be empty.
    void addRow(const vector<string> &row);

    void setRow(size_t row, size_t col, const string &value);

    /// @brief Returns the literal length of the table when printed
    /// @details As it may be called often, its result is stored in the length variable.
    [[nodiscard]] size_t getLength() const { return length; }

    [[nodiscard]] size_t getHeight() const { return (rows.size() + hasTitle + hasHeaders) * 2 + 1; }

    /// @brief Widens a column
    /// @details By default, the column width is the length of the header plus one space in each side.
    /// @param col The column to be widened.
    /// @param width The number of extra spaces that will be added to each side.
    /// Note that the extra width will be this parameter times two.
    void widenColumn(size_t col, int width);

    void enableFold() { foldEnabled = true; }
    void disableFold() { foldEnabled = false; }
};
#endif //WINDOW_H
