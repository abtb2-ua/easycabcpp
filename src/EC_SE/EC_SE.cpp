#include <Ncurses/Common.h>
#include <Ncurses/Window.h>
#include <Socket/SocketHandler.h>
#include <arpa/inet.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dotenv.h>
#include <glib.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include "Common.h"

void communicateWithDE();

inline bool getStop();
inline void setStop(); // As stop indicates the end of the program, it just needs to be set to true

inline int getCounter();
inline void setCounter(int value);
inline void decrementCounter();

inline bool isMoving();
inline void setMoving(bool value);

mutex mut;
bool stop = false;
int counter = 0;
bool moving = false;

unique_ptr<Window> popUp_window;
unique_ptr<Window> menu_window;

int main() {
    dotenv::init(dotenv::Preserve);

    initCurses();

    const int maxX = getmaxx(stdscr);
    const int maxY = getmaxy(stdscr);
    constexpr int menuHeight = 7;
    constexpr int menuWidth = 30;
    constexpr int popUpHeight = 5;
    constexpr int popUpWidth = 26;

    popUp_window =
            make_unique<Window>(popUpHeight, popUpWidth, (maxY - popUpHeight) / 2, (maxX - popUpWidth) / 2, 2, 1);
    popUp_window->setScroll(false);

    menu_window = make_unique<Window>(menuHeight, menuWidth, (maxY - menuHeight) / 2, (maxX - menuWidth) / 2, 1, 2);
    menu_window->setScroll(false);

    Menu menu("", {"Minor inconvenience", "Inconvenience", "Major inconvenience", "Exit"});
    menu.setPrefix("[$] ");

    timeout(0);
    keypad(stdscr, true);

    // refresh();
    menu_window->showBox();
    popUp_window->showBox();

    thread thread(communicateWithDE);
    thread.detach();

    bool lastMoving = true;
    bool error = true;

    while (!getStop()) {
        if (isMoving()) {
            if (!lastMoving) {
                menu_window->showBox();
            }
            menu_window->clear();
            menu_window->addMenu(menu);
            menu_window->show();
            lastMoving = true;
        } else {
            if (lastMoving) {
                popUp_window->showBox();
            }
            // popUp_window->showBox();
            popUp_window->clear();
            popUp_window->addString(centerString("Taxi is not moving", popUpWidth - 3), COLOR_PAIR(COLOR::RED));
            popUp_window->show();
            lastMoving = false;
        }

        const int c = getch();
        if (c == 'q' || c == 'Q') {
            setStop();
            error = false;
            break;
        }

        if (!isMoving()) {
            continue;
        }

        switch (c) {
            case KEY_DOWN:
                menu.hoverNext();
                break;

            case KEY_UP:
                menu.hoverPrevious();
                break;

            case ' ':
            case '\n':
                if (menu.getHovered() == menu.getOptions().size() - 1) {
                    setStop();
                    error = false;
                    break;
                } else if (menu.getHovered() == 0) {
                    setCounter(5);
                } else if (menu.getHovered() == 1) {
                    setCounter(15);
                } else if (menu.getHovered() == 2) {
                    setCounter(30);
                }
                break;

            default:
                break;
        }
    }

    timeout(20 * 1000);
    endCurses(error ? "DE disconnected" : "");
    return 0;
}

void communicateWithDE() {
    SocketHandler socket;
    const string ip = dotenv::getenv("DE_IP", "127.0.0.1");
    const int port = envInt("DE_PORT", 9001);

    if (!socket.connectTo(Address(ip, port)) || !socket.setTimeout(0)) {
        // setStop();
        return;
    }

    while (socket.readFromSocket()) {
        if (!socket.hasData()) {
            break;
        }

        setMoving(static_cast<bool>(socket.getData()[0]));
        int _counter = getCounter();
        memcpy(socket.getBuffer().data(), &_counter, sizeof(_counter));
        socket.writeToSocket();

        sleep(1);
        if (getCounter() != 0) {
            decrementCounter();
        }
    }

    setStop();
}

int getCounter() {
    lock_guard lock(mut);
    return counter;
}

void setCounter(const int value) {
    lock_guard lock(mut);
    counter = value;
}

void decrementCounter() {
    lock_guard lock(mut);
    counter--;
}

void setMoving(const bool value) {
    lock_guard lock(mut);
    moving = value;
}

bool isMoving() {
    lock_guard lock(mut);
    return moving;
}

bool getStop() {
    lock_guard lock(mut);
    return stop;
}

void setStop() {
    lock_guard lock(mut);
    stop = true;
}
