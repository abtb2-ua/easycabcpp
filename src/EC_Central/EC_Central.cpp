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

char session[UUID_LENGTH];


int main() {
    dotenv::init(dotenv::Preserve);

    initSession();

    if (envTrue("RESET_DB")) {
        readFile();
    }
    return 0;
    const PipeHandler pipeHandler;

    if (!createProcess(ncursesGui,
                       static_cast<PipeHandler::PipeReader[]>(pipeHandler.getReader()))) {
        codeLog(ERROR::UNDEFINED, "Error creating process");
    }

    auto writer = pipeHandler.getWriter();

    sleep(5);

    bool b = writer.sendCode(SocketHandler::CODE::ACK);
    return 0;

    const pid_t auth_process = fork();

    if (auth_process == -1) { g_error("Error forking"); }

    if (auth_process == 0) {
        // Socket module entrypoint
    }

    // Kafka module entrypoint

    return 0;
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
    char buffer[1024] = {};
    buffer[0] = 0;
    buffer[1] = 1;
    const cppkafka::Buffer smthBuffer(reinterpret_cast<const char *>(&buffer), sizeof(buffer));

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
                        "INSERT INTO session(id) VALUES ('{}');", session));

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

    memcpy(session, row.value()[0].c_str(), UUID_LENGTH);
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
