#include <Ncurses/Common.h>
#include <Ncurses/Window.h>
#include <Protocols.h>
#include <Socket/SocketHandler.h>
#include <array>
#include <cppkafka/cppkafka.h>
#include <dotenv.h>
#include <iostream>
#include <queue>
#include <thread>

#include "Common.h"
#include "Logs.h"

using namespace std;

void handleGUI();
void listenKafka();
void listenSensor(SocketHandler &socketHandler);
void authenticate();

deque<prot::Log> logs;

int main() {
    dotenv::init(dotenv::Preserve);

    SocketHandler socketHandler;
    if (!socketHandler.openSocket(envInt("DE_LISTEN_PORT"))) {
        codeLog(ERROR::UNDEFINED, "Failed to open socket");
    }

    authenticate();
    return 0;

    thread guiThread(handleGUI);
    thread kafkaThread(listenKafka);
    thread sensorThread(listenSensor, ref(socketHandler));


    guiThread.join();

    return 0;
}

void listenKafka() {}

void listenSensor(SocketHandler &socketHandler) {}

void authenticate() {
    SocketHandler socketHandler;
    Address central;
    central.setIp(dotenv::getenv("CENTRAL_IP", "localhost"));
    central.setPort(envInt("CENTRAL_LISTEN_PORT"));

    if (!socketHandler.connectTo(central)) {
        codeLog(ERROR::UNDEFINED, "Failed to connect to central");
    }

#define codeError(code, ...)                                                                                           \
    {                                                                                                                  \
        socketHandler.sendControlChar(CONTROL_CHAR::EOT);                                                              \
        codeLog(code, __VA_ARGS__);                                                                                    \
    }

    if (!socketHandler.setTimeout(2)) {
        codeError(ERROR::UNDEFINED, "Failed to set timeout");
    }

    for (int i = 0; i < 3; i++) {
        socketHandler.sendControlChar(CONTROL_CHAR::ENQ);

        if (!socketHandler.readFromSocket()) {
            if (socketHandler.getError() == SocketHandler::READ_ERROR::TIMEOUT) {
                codeLog(WARNING::UNDEFINED, "Timeout");
            } else {
                codeError(ERROR::UNDEFINED, "Error reading from socket");
            }
        }

        if (socketHandler.hasControlChar() && socketHandler.getControlChar() == CONTROL_CHAR::ACK) {
            break;
        }

        if (i == 2) {
            codeError(ERROR::UNDEFINED, "Couldn't authenticate. Try limit reached");
        }
        codeLog(WARNING::UNDEFINED, "Couldn't authenticate, retrying");
    }

    const short id = envInt("DE_ID");
    memcpy(socketHandler.getBuffer().data(), &id, sizeof(short));
    socketHandler.writeToSocket();

    if (!socketHandler.readFromSocket()) {
        if (socketHandler.getError() == SocketHandler::READ_ERROR::TIMEOUT) {
            codeLog(WARNING::UNDEFINED, "Timeout");
        } else {
            codeError(ERROR::UNDEFINED, "Error reading from socket");
        }
    }

    if (socketHandler.hasControlChar() && socketHandler.getControlChar() != CONTROL_CHAR::ACK) {
        codeError(ERROR::UNDEFINED, "Couldn't authenticate");
    }

    codeLog(MESSAGE::UNDEFINED, "Authenticated");
    socketHandler.sendControlChar(CONTROL_CHAR::EOT);
}

void handleGUI() {
    initCurses();
    startColors();

    array<string, 8> values = {};
    array<string, 8> headers = {
            "Position", "Going towards", "Service",    "Taxi status",
            "Reason",   "Sensor status", "Last order", "Last order status",
    };

    constexpr int menuWidth = 20;
    constexpr int menuHeight = 5;
    const int height = getmaxy(stdscr);
    const int width = getmaxx(stdscr);
    bool showTable = false;

    Window win_log(height, width - menuWidth, 0, 0, 0, 0);
    Window win_table(height, width - menuWidth, 0, 0, 0, 0);
    Window win_menu(menuHeight, menuWidth, 0, width - menuWidth, 1, 1);
    win_menu.setTimeout(0);
    win_menu.showBox();
    win_menu.setScroll(false);

    Menu menu("", {"Log view", "Table view", "Exit"});
    menu.setPrefix(" [$] ");
    menu.select();

    Table table(2, {20, 35});
    table.addEmptyRows(headers.size());
    for (size_t i = 0; i < headers.size(); i++) {
        table.setRow(i, 0, headers[i]);
    }

    while (true) {
        const int ch = win_menu.getChar();

        if (ch == 'q' || ch == 'Q') {
            break;
        } else if (ch == KEY_DOWN) {
            menu.hoverNext();
        } else if (ch == KEY_UP) {
            menu.hoverPrevious();
        } else if (ch == '\n' || ch == ' ') {
            if (menu.getHovered() == 2) {
                break;
            }
            showTable = menu.getHovered() == 1;
        }

        win_menu.clear();
        win_menu.addMenu(menu);
        win_menu.show();

        if (!showTable) {
            win_log.clear();
            for (const auto &log: logs) {
                win_log.addLog(log);
            }
            win_log.show();
        } else {
            win_table.clear();
            for (size_t i = 0; i < headers.size(); i++) {
                table.setRow(i, 1, values[i]);
            }
            win_table.addTables({{&table}}, true, true);
            win_table.show();
        }
    }

    endCurses();
}
