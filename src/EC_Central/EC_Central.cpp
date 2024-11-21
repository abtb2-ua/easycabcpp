#include <glib-2.0/glib.h>
#include <iostream>
#include <fstream>
#include <cppkafka/cppkafka.h>
#include <dotenv.h>
#include <variant>

#include "MySQL/DbConnectionHandler.h"
#include "Common.h"
#include "GUI.h"
#include "PipeHandler.h"
#include "Logs.h"
#include "../Common/Logs.h"

using namespace std;
using namespace code_logs;

void initSession();

void readFile();

array<char, UUID_LENGTH> session = {};


int main() {
    const PipeHandler ph_toGui;
    const PipeHandler ph_fromGui;

    ncursesGui(ph_fromGui.getWriter(), ph_toGui.getReader());

    int b = 3;
    function<void(int)> f = [b](int a) {
        cout << a + b << endl;
    };
    f(3);
    return 0;

    dotenv::init(dotenv::Preserve);

    initSession();

    if (envTrue("RESET_DB")) {
        readFile();
    }
    // const PipeHandler ph_toGui;
    // const PipeHandler ph_fromGui;

#define safe_createProcess(func, ...) \
    if (!createProcess(func, __VA_ARGS__)) { \
        codeLog(ERROR::CREATING_PROCESS); \
        exit(0); \
    }

    safe_createProcess(function(ncursesGui), move(ph_fromGui.getWriter()), move(ph_toGui.getReader()));

    array<byte, PipeHandler::BUFFER_SIZE> buffer = {};
    // ph_toGui.getWriter().sendCode(CONTROL_CHAR::ACK);
    // fork();
    // ph_fromGui.getReader().readFromPipe(buffer);
    cout << "Received: " << static_cast<int>(buffer[0]) << endl;;

    return 0;

    // PipeHandler ph;
    // auto writer = ph.getWriter();
    // auto reader = ph.getReader();
    //
    // array<byte, PipeHandler::BUFFER_SIZE> buffer = {};
    // cout << static_cast<int>(writer.sendCode(CONTROL_CHAR::ACK)) << endl;
    // cout << static_cast<int>(reader.readFromPipe(buffer)) << endl;
    // cout << "Received: " << static_cast<int>(buffer[0]) << endl;

    // sleep(2);
    // cout << "Sending writer" << endl;
    // PipeHandler ph_fromGui;
    // auto writer = ph_toGui.getWriter();
    // auto reader = ph_fromGui.getReader();
    // array<byte, PipeHandler::BUFFER_SIZE> buffer = {};
    // buffer[0] = byte{32};
    // memcpy(buffer.data() + 1, PipeHandler::serialize(ph_fromGui.getWriter()).data(), PipeHandler::BUFFER_SIZE - 1);
    // writer.writeToPipe(buffer);
    // sleep(2);
    // cout << "Sending EOT" << endl;
    // writer.sendCode(CONTROL_CHAR::EOT);
    // buffer = {};
    // cout << "Central: Reading" << endl;
    // ph_fromGui.getReader().readFromPipe(buffer);
    // cout << "Central: Read" << endl;
    // if (buffer[0] != static_cast<byte>(CONTROL_CHAR::EOT))
    //     cout << "Success" << endl;
    // else
    //     cout << "Failed" << endl;
    // // const auto writer = ph_toGui.getWriter();
    // //
    // // if (!)
    //
    // cout << "Exiting central" << endl;
    // return 0;
}

// void db_schema() {
//     DbConnectionHandler dbConnectionHandler;
//     if (!dbConnectionHandler.connect()) {
//         g_error("Error connecting to database");
//     }
//
//     auto results = dbConnectionHandler.query("select * from t2", false);
//
//     if (!results) {
//         g_error("Error querying database: %s", results.error().c_str());
//     }
//
//     for (auto &result: results.value()) {
//         for (auto &row: result) {
//             for (auto &value: row) {
//                 cout << value << " ";
//             }
//             cout << endl;
//         }
//     }
// }

void kafka_schema() {
    cppkafka::Producer producer(getConfig(false));


    const string key = "some_key";
    byte buffer[1024] = {};
    buffer[0] = byte{0};
    buffer[1] = byte{1};
    const cppkafka::Buffer smthBuffer(reinterpret_cast<const byte *>(&buffer), sizeof(buffer));

    // Build and produce the message
    producer.produce(cppkafka::MessageBuilder("requests").key(key).payload(smthBuffer));
    producer.flush();
}

void initSession() {
    DbConnectionHandler dbConnectionHandler;

    if (!dbConnectionHandler.connect()) {
        codeLog(ERROR::UNDEFINED, "Error connecting to database");
    }

    if (envTrue("RESET_DB")) {
        generate_unique_id(session);

        const auto exp = dbConnectionHandler.query(
            fmt::format("CALL Reset_DB();"
                        "INSERT INTO session(id) VALUES ('{}');", session.data()));

        if (!exp) {
            codeLog(ERROR::UNDEFINED, "Error initializing session: " + exp.error());
        }
        return;
    }

    auto exp = dbConnectionHandler.query("SELECT id FROM session");
    if (!exp) {
        codeLog(ERROR::UNDEFINED, "Error initializing session: %s", exp.error().c_str());
    }

    const auto row = exp.value().getRow();

    if (!row || row.value().empty()) {
        codeLog(ERROR::UNDEFINED, "Error initializing session: No session found");
    }

    // -1 because length does not include null terminator
    if (row.value()[0].length() != UUID_LENGTH - 1) {
        codeLog(ERROR::UNDEFINED, "Error initializing session: Invalid session id");
    }

    memcpy(session.data(), row.value()[0].c_str(), UUID_LENGTH);
}

void readFile() {
    const string fileName = dotenv::getenv("LOCATIONS_FILE", "");

    if (fileName.empty()) {
        codeLog(ERROR::UNDEFINED, "Error reading file: LOCATIONS_FILE not set");
    }

    ifstream file(fileName);
    string line;
    DbConnectionHandler dbConnectionHandler;

    if (!file.is_open()) {
        codeLog(ERROR::UNDEFINED, "Error opening file");
    }

    if (!dbConnectionHandler.connect()) {
        codeLog(ERROR::UNDEFINED, "Error connecting to database");
    }

    while (getline(file, line)) {
        stringstream ss(line);
        char id;
        int x, y;
        char buffer;
        ss >> id >> buffer >> x >> buffer >> y;

        if (id < 'A' || id > 'Z' || --x < 0 || x > 19 || --y < 0 || y > 19) {
            g_warning("Invalid location: %c %d %d", id, x + 1, y + 1);
            continue;
        }

        string query = fmt::format("INSERT INTO locations(id, x, y) VALUES ('{}', {}, {}) "
                                   "ON DUPLICATE KEY UPDATE x = {}, y = {};",
                                   id, x, y, x, y);

        const auto exp = dbConnectionHandler.query(query);

        if (!exp) {
            g_warning("Error inserting location (%c, %d, %d): %s", id, x, y, exp.error().c_str());
        }
    }
}
