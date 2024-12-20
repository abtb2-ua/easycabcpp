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

void handleGUI();
void listenKafka();
void listenSensor(const SocketHandler &socketHandler);
void attend(SocketHandler &sensor, ProducerWrapper &producer);
void authenticate();
void step();
void ping();

int calculateDistance(const Coordinate &a, const Coordinate &b);
// Returns the mirror of n with respect to reference
// Example: mirror(11, 10) = 9, mirror(2, 10) = 18
int mirror(int n, int reference);

deque<Log> logs;

mutex mut;
bool stop;

short id;
string session;

Coordinate pos = {0, 0};
Coordinate dest = {0, 0};
bool orderedToStop = false;

template<typename T>
T peek(T &value) {
    lock_guard lock(mut);
    return value;
}

template<typename T>
void poke(T &value, const T &newValue) {
    lock_guard lock(mut);
    value = newValue;
}

#define log(code, message)                                                                                             \
    {                                                                                                                  \
        mut.lock();                                                                                                    \
        logs.emplace_back(code, message);                                                                              \
        mut.unlock();                                                                                                  \
    }

int main() {
    dotenv::init(dotenv::Preserve);
    id = envInt("DE_ID");

    SocketHandler socketHandler;
    if (!socketHandler.openSocket(envInt("DE_LISTEN_PORT", 9001))) {
        codeLog(ERROR::UNDEFINED, "Failed to open socket");
    }

    authenticate();

    thread sensorThread(listenSensor, ref(socketHandler));
    thread guiThread(handleGUI);
    thread kafkaThread(listenKafka);
    thread pingThread(ping);


    sensorThread.detach();
    guiThread.join();
    kafkaThread.join();
    pingThread.join();

    cout << "Exiting..." << endl;

    return 0;
}

void listenKafka() {
    ConsumerWrapper consumer(to_string(id));
    ProducerWrapper producer;
    consumer.subscribe({"responses"});

    while (!peek(stop)) {
        auto opt = consumer.consume<prot::Message>();
        if (!opt.has_value()) {
            lock_guard lock(mut);
            if (consumer.get_error()) {
                logs.emplace_back(MESSAGE::DEBUG, "Failed to consume message: " + consumer.get_error().to_string());
            }
            continue;
        }

        prot::Message &message = opt.value();
        if (message.getTaxiId() != id || !holds_alternative<SUBJ_RESPONSE>(message.getSubject())) {
            continue;
        }

        switch (get<SUBJ_RESPONSE>(message.getSubject())) {
            case SUBJ_RESPONSE::TAXI_GO_TO:
                log(MESSAGE::UNDEFINED, "Received order to go to " + message.getCoord().toString());
                if (peek(pos) == message.getCoord()) {
                    log(MESSAGE::UNDEFINED, "Already there");
                    message.setSubject(SUBJ_REQUEST::TAXI_DESTINATION_REACHED);
                    message.setCoord(peek(pos));
                    message.setSession(session);
                    producer.produce("requests", message);
                } else {
                    poke(dest, message.getCoord());
                }
                break;

            case SUBJ_RESPONSE::TAXI_STOP:
                log(MESSAGE::UNDEFINED, "Received order to stop");
                poke(orderedToStop, true);
                break;

            case SUBJ_RESPONSE::TAXI_CONTINUE:
                log(MESSAGE::UNDEFINED, "Received order to continue");
                poke(orderedToStop, false);
                break;

            case SUBJ_RESPONSE::TAXI_RETURN_TO_BASE:
                log(MESSAGE::UNDEFINED, "Received order to return to base");
                poke(dest, Coordinate{0, 0});
                break;

            case SUBJ_RESPONSE::TAXI_DISCONNECT:
                log(MESSAGE::UNDEFINED, "Received order to disconnect");
                poke(stop, true);
                break;

            default:
                break;
        }
    }
}

void listenSensor(const SocketHandler &socketHandler) {
    ProducerWrapper producer;
    prot::Message message;
    message.setTaxiId(id);
    message.setSession(session);

    while (!peek(stop)) {
        SocketHandler sensor = socketHandler.acceptConnections();

        if (!sensor.setTimeout(2)) {
            lock_guard lock(mut);
            logs.emplace_back(WARNING::UNDEFINED, "Failed to set timeout");
            continue;
        }

        {
            lock_guard lock(mut);
            logs.emplace_back(MESSAGE::UNDEFINED, "Sensor connected");
        }

        message.setSubject(SUBJ_REQUEST::TAXI_READY);
        producer.produce("requests", message);

        attend(sensor, producer);

        {
            lock_guard lock(mut);
            logs.emplace_back(WARNING::UNDEFINED, "Sensor disconnected");
        }

        message.setSubject(SUBJ_REQUEST::TAXI_NOT_READY);
        producer.produce("requests", message);
    }

    message.setSubject(SUBJ_REQUEST::ORDER_TAXI_DISCONNECT);
    producer.produce("requests", message);
}


void attend(SocketHandler &sensor, ProducerWrapper &producer) {
    prot::Message message;
    message.setTaxiId(id);
    message.setSession(session);

    do {
        bool moving = peek(pos) != peek(dest);
        int waitTime;
        memcpy(&waitTime, sensor.getData().data(), sizeof(int));

        log(MESSAGE::UNDEFINED, "Received wait time: " + to_string(waitTime));
        if (waitTime > 0) {
            message.setSubject(SUBJ_REQUEST::TAXI_UPDATE_WAIT_TIME);
            message.setData(waitTime - 1);
            producer.produce("requests", message);
        } else if (moving && !peek(orderedToStop)) {
            step();
            {
                lock_guard lock(mut);
                logs.emplace_back(MESSAGE::UNDEFINED, "Moved to " + pos.toString());
            }
            moving = peek(pos) != peek(dest);
            message.setSubject(moving ? SUBJ_REQUEST::TAXI_MOVE : SUBJ_REQUEST::TAXI_DESTINATION_REACHED);
            message.setCoord(peek(pos));
            producer.produce("requests", message);
        }

        sensor.getBuffer()[0] = static_cast<byte>(moving && !peek(orderedToStop));
        if (!sensor.writeToSocket()) {
            break;
        }
    } while (!peek(stop) && sensor.readFromSocket());

    {
        lock_guard lock(mut);
        logs.emplace_back(WARNING::UNDEFINED, "Sensor disconnected");
    }
}

void ping() {
    ProducerWrapper producer;
    prot::Message message;
    message.setTaxiId(id);
    message.setSession(session);

    while (!peek(stop)) {
        message.setSubject(SUBJ_REQUEST::TAXI_PING);
        producer.produce("requests", message);
        this_thread::sleep_for(1s);
    }
}

void authenticate() {
    // ENQ -> ACK
    // ID proposal -> ACK && session
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

    memcpy(socketHandler.getBuffer().data(), &id, sizeof(short));
    socketHandler.writeToSocket();

    if (!socketHandler.readFromSocket()) {
        if (socketHandler.getError() == SocketHandler::READ_ERROR::TIMEOUT) {
            codeLog(WARNING::UNDEFINED, "Timeout");
        } else {
            codeError(ERROR::UNDEFINED, "Error reading from socket");
        }
    }

    if (!socketHandler.hasData() || socketHandler.getData()[0] != static_cast<byte>(CONTROL_CHAR::ACK)) {
        codeError(ERROR::UNDEFINED, "Couldn't authenticate");
    }

    session.resize(36);
    memcpy(session.data(), socketHandler.getData().data() + 1, 36);

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

    while (!peek(stop)) {
        const int ch = win_menu.getChar();

        if (ch == 'q' || ch == 'Q') {
            break;
        }

        if (ch == KEY_DOWN) {
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
            lock_guard lock(mut);
            win_log.clear();
            for (const auto &log: logs) {
                win_log.addLog(log);
            }
            win_log.show();
        } else {
            lock_guard lock(mut);
            win_table.clear();
            for (size_t i = 0; i < headers.size(); i++) {
                table.setRow(i, 1, values[i]);
            }
            win_table.addTables({{&table}}, true, true);
            win_table.show();
        }
    }

    poke(stop, true);
    endCurses();
}

int mirror(const int n, const int reference) { return 2 * reference - n; }

int calculateDistance(const Coordinate &a, const Coordinate &b) {
    int dist_x = abs(a.x - b.x);
    int dist_y = abs(a.y - b.y);
    return min(dist_x, mirror(dist_x, 10)) + min(dist_y, mirror(dist_y, 10));
}

void step() {
    static array<Coordinate, 8> directions = {{
            {1, 0},
            {1, 1},
            {0, 1},
            {-1, 1},
            {-1, 0},
            {-1, -1},
            {0, -1},
            {1, -1},
    }};

    const Coordinate localPos = peek(pos);
    const Coordinate localDest = peek(dest);

    if (localPos == localDest) {
        return;
    }

    Coordinate bestMove = {0, 0};
    int bestDistance = numeric_limits<int>::max();

    for (const auto direction: directions) {
        Coordinate nextMove = {localPos.x + direction.x, localPos.y + direction.y};
        nextMove.x %= 20;
        nextMove.y %= 20;
        if (nextMove.x < 0) {
            nextMove.x += 20;
        }
        if (nextMove.y < 0) {
            nextMove.y += 20;
        }
        const int distance = calculateDistance(nextMove, localDest);

        if (distance < bestDistance) {
            bestDistance = distance;
            bestMove = nextMove;
        }
    }

    poke(pos, bestMove);
}
