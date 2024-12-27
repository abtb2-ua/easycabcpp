#include <cppkafka/cppkafka.h>
#include <dotenv.h>
#include <fstream>
#include <iostream>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <variant>

#include "AuthServer.h"
#include "Common.h"
#include "KafkaServer.h"
#include "Logs.h"
#include "MySQL/DbConnectionHandler.h"
#include "Protocols.h"

using namespace std;
using namespace code_logs;
using namespace prot;

void initSession();

void readFile();

void cleanUp();

void centralLogHandler(ProducerWrapper &producer, const LogType &code, const string &message, bool printBottom);

string session = "";
pid_t kafkaServerPid = -1;
pid_t authServerPid = -1;

int main() {
    dotenv::init(dotenv::Preserve);
    ProducerWrapper producer;

    setDefaultLogHandler([&producer](const LogType &code, const string &message) {
        centralLogHandler(producer, code, message, false);
    });

    initSession();

    if (envTrue("RESET_DB")) {
        readFile();
    }

    kafkaServerPid = createProcess(KafkaServer::start);
    authServerPid = createProcess(AuthServer::start);

    auto handler = [](int) {
        cout << "Sending SIGINT to child processes" << endl;
        kill(kafkaServerPid, SIGINT);
        kill(authServerPid, SIGINT);
        kafkaServerPid = authServerPid = -1;
        cleanUp();
    };

    signal(SIGINT, handler);
    signal(SIGTERM, handler);

    cleanUp();
}

void cleanUp() {
    int status;
    const pid_t pid = waitpid(-1, &status, 0);
    const pid_t other = pid == authServerPid ? kafkaServerPid : authServerPid;
    const string firstProcess = pid == authServerPid ? "Auth server" : "Kafka server";
    const string secondProcess = pid == authServerPid ? "Kafka server" : "Auth server";

    cout << "[Parent process] " << firstProcess << " exited with status: " << status << endl;
    if (other != -1) {
        cout << "[Parent process] " << "Sending SIGINT to " << secondProcess << endl;
        kill(other, SIGINT);
    }
    waitpid(other, &status, 0);
    cout << "[Parent process] " << secondProcess << " exited with status: " << status << endl;
    cout << "[Parent process] " << "Exiting central" << endl;
    exit(0);
}

void initSession() {
    DbConnectionHandler db;

    if (!db.connect()) {
        codeLog(ERROR::DATABASE, "Error connecting to database");
    }

    if (envTrue("RESET_DB")) {
        session = generate_unique_id();
        const string query = fmt::format("CALL Reset_DB(); INSERT INTO session(id) VALUES ('{}');", session.data());

        const auto exp = db.query(query);

        if (!exp.has_value()) {
            codeLog(ERROR::SESSION, "Error initializing session: " + exp.error());
        }

        codeLog(MESSAGE::SESSION_INITIALIZED, "Session initialized");
        return;
    }

    auto exp = db.query("SELECT id FROM session");
    if (!exp.has_value()) {
        codeLog(ERROR::SESSION, "Error initializing session: %s", exp.error().c_str());
    }

    const auto row = exp.value().getRow();

    if (row.empty()) {
        codeLog(ERROR::SESSION_NOT_FOUND, "Error initializing session: No session found");
    }

    session = row[0];
    codeLog(MESSAGE::RECONNECTED_TO_SESSION, "Reconnected to session");
}

void readFile() {
    const string fileName = dotenv::getenv("LOCATIONS_FILE", "");

    if (fileName.empty()) {
        codeLog(ERROR::LOCATIONS_FILE_NOT_PROVIDED, "Error reading file: LOCATIONS_FILE not set");
    }

    ifstream file(fileName);
    string line;
    DbConnectionHandler dbConnectionHandler;

    if (!file.is_open()) {
        codeLog(ERROR::OPENING_FILE, "Error opening file");
    }

    if (!dbConnectionHandler.connect()) {
        codeLog(ERROR::DATABASE, "Error connecting to database");
    }

    while (getline(file, line)) {
        stringstream ss(line);
        char id;
        int x, y;
        char buffer;
        ss >> id >> buffer >> x >> buffer >> y;

        if (id < 'A' || id > 'Z' || --x < 0 || x > 19 || --y < 0 || y > 19) {
            codeLog(WARNING::UNDEFINED, format("Invalid location: {} {} {}", id, x + 1, y + 1));
            continue;
        }

        string query = format("INSERT INTO locations(id, x, y) VALUES ('{0}', {1}, {2}) "
                              "ON DUPLICATE KEY UPDATE x = {1}, y = {2};",
                              id, x, y);

        const auto exp = dbConnectionHandler.query(query);

        if (!exp.has_value()) {
            codeLog(WARNING::UNDEFINED, format("Error inserting location ({}, {}, {}): {}", id, x, y, exp.error()));
        }
    }
}

void centralLogHandler(ProducerWrapper &producer, const LogType &code, const string &message, const bool printBottom) {
    static const unordered_set important = {
            MESSAGE::LOADED_LOC_FILE,
    };

    producer.produce(dotenv::getenv("TOPIC_LOG", "logs"), Log(code, message, printBottom));

    if (!holds_alternative<MESSAGE>(code) || important.contains(get<MESSAGE>(code))) {
        defaultLogHandler(code, message);
    }
}
