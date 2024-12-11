//
// Created by ab-flies on 15/11/24.
//

#include <chrono>
#include <dotenv.h>
#include <list>
#include <memory>
#include <ncurses.h>
#include <ranges>

#include "Common.h"
#include "Ncurses/Common.h"
#include "Ncurses/Window.h"
#include "Protocols.h"

#define WIDTH 31
#define HEIGHT 20

using namespace std;
using namespace code_logs;
using namespace prot;

void initGlobals();

void initWindows();

void printMenu();

void printLogs();

void printTableView();

bool handleUserInput(int ch);

void listenKafka();

template<typename T, size_t N>
bool operator==(const array<T, N> &lhs, const array<T, N> &rhs) {
    for (size_t i = 0; i < N; i++) {
        if (lhs[i] != rhs[i])
            return false;
    }
    return true;
}

unique_ptr<Window> win_menu;
unique_ptr<Window> win_top;
unique_ptr<Window> win_bottom;
unique_ptr<Window> win_tables;

ConsumerWrapper consumer;
ProducerWrapper producer;

unique_ptr<Menu> menu_views, menu_actions, menu_options;
unique_ptr<Table> table_locations, table_taxis, table_customers;

// Used deque instead of list for performance reasons. However, conceptually, they are lists
deque<Log> lst_topLogs, lst_bottomLogs;

// Used to just print once the box for the different views, avoiding flickering
bool boxShown = true; // false = log view, true = table view

// Menu related variables
unordered_map<string, size_t> actions;
string arg_taxi = "__";
string arg_coord = "____";
bool actionExecuted = false;
bool frozen = false;

string err = "";

int main() {
    dotenv::init(dotenv::Preserve);

    initCurses();
    initGlobals();
    win_menu->setTimeout(0);

    printMenu();

    while (true) {
        listenKafka();

        const int ch = win_menu->getChar();

        if (ch != ERR && !handleUserInput(ch))
            break;

        printMenu();

        if (menu_views->getHovered() == 0) {
            printLogs();
        } else {
            printTableView();
        }
    }

    endCurses(err);
    cout << "Exiting gui" << endl;

    return 0;
}

void listenKafka() {
    const Log log = consumer.consume<Log>(0ms).value_or(Log());

    if (frozen)
        return;

    if (log.getMessage().empty()) {
        if (const auto error = consumer.get_error(); error) {
            lst_topLogs.push_back(Log(ERROR::UNDEFINED, "Can't listen for logs: " + error.to_string()));
        }
        return;
    }

    auto &lst = log.getPrintAtBottom() ? lst_bottomLogs : lst_topLogs;

    lst.push_back(log);
    if (static_cast<int>(lst.size()) > HEIGHT) {
        lst.pop_front();
    }
}


void printLogs() {
    win_bottom->clear();
    win_top->clear();

    if (frozen) {
        win_bottom->attrOn(A_DIM);
        win_top->attrOn(A_DIM);
    } else {
        win_bottom->attrOff(A_DIM);
        win_top->attrOff(A_DIM);
    }

    for (const auto &log: lst_topLogs) {
        win_top->addLog(log);
    }
    for (const auto &log: lst_bottomLogs) {
        win_bottom->addLog(log);
    }

    if (boxShown) {
        win_bottom->showBox();
        boxShown = false;
    }

    win_top->show();
    win_bottom->show();
}

void printMenu() {
    static bool changedViews = false;

    if (menu_views->getHovered() != 0)
        changedViews = true;


    win_menu->clear();

    win_menu->addMenu(*menu_views);
    if (!changedViews) {
        win_menu->addLine("(Press tab to change view)", A_DIM);
    } else {
        win_menu->addLine();
    }

    win_menu->addLine();

    menu_actions->setOption(actions["Freeze"], frozen ? "Unfreeze" : "Freeze");
    win_menu->addMenu(*menu_actions);

    if (menu_actions->isSelected()) {
        win_menu->addLine();
        string taxiOpt = fmt::format("Taxi: {}", arg_taxi);
        string coordOpt = fmt::format("Coordinate: [{}, {}]", arg_coord.substr(0, 2), arg_coord.substr(2));

        if (menu_actions->getHovered() == actions["Go to"]) {
            menu_options->setOptions({taxiOpt, coordOpt, "Execute"});
        } else {
            menu_options->setOptions({taxiOpt, "Execute"});
        }

        win_menu->addMenu(*menu_options);
        win_menu->addLine("(Press 'b' to cancel)", A_DIM);
    }

    if (actionExecuted) {
        win_menu->addLine();
        win_menu->addLine("Message sent to central", COLOR_PAIR(COLOR::GREEN));
    }

    win_menu->show();
}

void printTableView() {
    win_tables->clear();
    win_tables->addTables({{table_locations.get(), table_customers.get()}, {table_taxis.get()}}, true, true);

    if (!boxShown) {
        win_tables->showBox();
        boxShown = true;
    }
    win_tables->show();
}

bool handleUserInput(const int ch) {
    actionExecuted = false;

    switch (ch) {
        case 'q':
        case 'Q':
            return false;

        case KEY_DOWN:
        case 's':
        case 'S':
            if (menu_actions->isSelected()) {
                menu_options->hoverNext();
            } else {
                menu_actions->hoverNext();
            }
            break;

        case KEY_UP:
        case 'w':
        case 'W':
            if (menu_actions->isSelected()) {
                menu_options->hoverPrevious();
            } else {
                menu_actions->hoverPrevious();
            }
            break;

        case '\t':
            menu_views->hoverNext();
            break;

        case '\n':
        case ' ':
            if (!menu_actions->isSelected()) {
                const size_t hovered = menu_actions->getHovered();

                if (hovered == actions["Close GUI"]) {
                    return false;
                }

                if (hovered == actions["Terminate central"]) {
                    // TODO: send message to central
                    actionExecuted = true;
                } else if (hovered == actions["Freeze"]) {
                    frozen = !frozen;
                } else if (hovered == actions["Clear"]) {
                    lst_topLogs.clear();
                    lst_bottomLogs.clear();
                } else {
                    menu_actions->select();
                }
                break;
            }

            if (menu_options->getHovered() != menu_options->getOptions().size() - 1 /* Execute */) {
                break;
            }

            // TODO: execute action
            actionExecuted = true;
            // Intentional fallthrough
        case 'b':
            if (menu_actions->isSelected()) {
                menu_actions->deselect();
                menu_options->hover(0);
                arg_taxi = "__";
                arg_coord = "____";
            }
            break;

        case KEY_BACKSPACE:
            if (menu_actions->isSelected()) {
                string *str = nullptr;

                if (menu_options->getHovered() == 0) {
                    // Taxi: __
                    str = &arg_taxi;
                } else if (menu_options->getHovered() == 1 && menu_actions->getHovered() == actions["Go to"]) {
                    // Coordinate: [__, __]
                    str = &arg_coord;
                }

                if (str) {
                    const auto it = ranges::find(*str, '_');
                    if (it == str->end())
                        str->back() = '_';
                    if (it != str->begin())
                        *(it - 1) = '_';
                }
            }
            break;

        default:
            if (menu_actions->isSelected() && ch >= '0' && ch <= '9') {
                string *str = nullptr;
                if (menu_options->getHovered() == 0) {
                    // Taxi: __
                    str = &arg_taxi;
                } else if (menu_options->getHovered() == 1 && menu_actions->getHovered() == actions["Go to"]) {
                    // Go to -> Coordinate: [__, __]
                    str = &arg_coord;
                }

                if (str) {
                    const auto it = ranges::find(*str, '_');
                    if (it != arg_taxi.end())
                        *it = ch;
                }
            }
            break;
    }

    return true;
}


void initWindows() {
    const int max_x = getmaxx(stdscr);
    const int max_y = getmaxy(stdscr);

    win_menu = make_unique<Window>(max_y, WIDTH, 0, 0, 2, 2);
    win_top = make_unique<Window>(HEIGHT, max_x - WIDTH, 0, WIDTH, 1, 1);
    win_tables = make_unique<Window>(max_y, max_x - WIDTH + 1, 0, WIDTH - 1, 1, 2);
    // +1 height and width accounts for the border
    win_bottom = make_unique<Window>(max_y - HEIGHT + 1, max_x - WIDTH + 1, HEIGHT - 1, WIDTH - 1, 1, 2);

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

void initGlobals() {
    initWindows();

    consumer.subscribe({ dotenv::getenv("TOPIC_LOG", "logs") });

    menu_views = make_unique<Menu>("VIEWS", vector<string>{"Logs", "Table"});
    menu_views->setPrefix("[$] ");
    menu_views->setHoverIndicator('X');

    menu_options = make_unique<Menu>("OPTIONS", vector<string>{});

    actions = {
            {"Go to", 0},   {"Return to base", 1}, {"Stop", 2},  {"Continue", 3},  {"Terminate central", 4},
            {Menu::GAP, 5}, {"Freeze", 6},         {"Clear", 7}, {"Close GUI", 8},
    };
    menu_actions = make_unique<Menu>("ACTIONS", vector<string>(actions.size(), Menu::GAP));

    for (const auto &[action, index]: actions) {
        menu_actions->setOption(index, action);
    }

    table_locations = make_unique<Table>(vector<string>{"ID", "Coordinate"}, "Locations");
    table_taxis = make_unique<Table>(vector<string>{"ID", "Coordinate", "Service", "Status"}, "Taxis");
    table_customers = make_unique<Table>(vector<string>{"ID", "Coordinate", "Destination", "Status"}, "Customers");
}
