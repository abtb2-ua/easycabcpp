//
// Created by ab-flies on 28/11/24.
//

#include "AuthServer.h"

#include <MySQL/DbConnectionHandler.h>
#include <dotenv.h>

AuthServer AuthServer::server;

void AuthServer::listenPetitions() {
    SocketHandler socketHandler;
    initProducer();

    setDefaultLogHandler([this](const LogType &code, const string &message) {
        lock_guard lock(this->mtx_producer);
        centralLogHandler(*this->producer, code, message, true);
    });

    const int port = envInt("CENTRAL_LISTEN_PORT", 8080);
    if (!socketHandler.openSocket(port)) {
        codeLog(ERROR::SETTING_SOCKET, "Error opening socket on port " + to_string(port));
    }
    codeLog(MESSAGE::LISTENING, "Listening connections on port " + to_string(port));

    int petitionId = 0;

    while (true) {
        SocketHandler client = socketHandler.acceptConnections();
        codeLog(MESSAGE::CONNECTION_ACCEPTED, "Connection with id " + to_string(petitionId) + " accepted");

        thread attend_thread(&AuthServer::attendPetition, this, move(client), petitionId);
        petitionId++;
        attend_thread.detach();
    }
}

void AuthServer::attendPetition(SocketHandler &&client, const int petitionId) {
    const string author = "Petition " + to_string(petitionId);

    if (!client.setTimeout(2)) {
        codeLog(author, WARNING::SETTING_TIMEOUT, "Error setting timeout");
        return;
    }

    while (true) {
        if (!client.readFromSocket()) {
            if (client.getError() == SocketHandler::READ_ERROR::TIMEOUT) {
                codeLog(author, WARNING::TIMEOUT, "Timeout reading from socket");
                break;
            }

            codeLog(author, WARNING::BAD_READING, "Error reading from socket");
            codeLog(author, MESSAGE::CONNECTION_CLOSED, "Connection closed");
            return;
        }

        if (client.hasData()) {
            short id;
            memcpy(&id, client.getData().data(), sizeof(short));

            bool reconnect = false;
            const bool added = (checkId(author, id, reconnect) && addTaxi(author, id, reconnect));

            client.getBuffer()[0] = static_cast<byte>(added ? CONTROL_CHAR::ACK : CONTROL_CHAR::NACK);
            memcpy(client.getBuffer().data() + 1, session.data(), session.size());
            client.writeToSocket();
            continue;
        }

        switch (client.getControlChar()) {
            case CONTROL_CHAR::ENQ: {
                lock_guard lock(this->mtx_db);
                const auto ch = (db.isConnected() || db.connect()) ? CONTROL_CHAR::ACK : CONTROL_CHAR::NACK;
                client.sendControlChar(ch);
                break;
            }

            case CONTROL_CHAR::EOT:
                codeLog(author, MESSAGE::UNDEFINED, "Connection closed");
                return;

            default:
                codeLog(MESSAGE::DEBUG, "Received default ", to_string(static_cast<int>(client.getControlChar())));
                break;
        }
    }
}

bool AuthServer::checkId(const string &author, const short taxiId, bool &reconnect) {
    const string query = "SELECT connected FROM taxis WHERE id = " + to_string(taxiId);

    this->mtx_db.lock();
    auto results = this->db.query(query);
    this->mtx_db.unlock();

    if (!results.has_value()) {
        codeLog(author, WARNING::UNDEFINED, "Couldn't run query: ", results.error());
        return false;
    }

    const auto row = results.value().getRow();

    if (row.empty()) {
        reconnect = false;
        return true;
    }

    reconnect = true;
    return row[0] == "0";
}

bool AuthServer::addTaxi(const string &author, const short taxiId, const bool reconnect) {
    const string query = reconnect ? "UPDATE taxis SET connected = TRUE WHERE id = " + to_string(taxiId)
                                   : "INSERT INTO taxis (id) VALUES (" + to_string(taxiId) + ")";

    this->mtx_db.lock();
    auto results = this->db.query(query);
    this->mtx_db.unlock();

    if (!results.has_value()) {
        codeLog(author, WARNING::UNDEFINED, "Couldn't run query: ", results.error());
        return false;
    }

    prot::Message message;
    codeLog(MESSAGE::UNDEFINED, session);
    message.setSession(session);
    message.setTaxiId(taxiId);
    message.setSubject(reconnect ? SUBJ_REQUEST::TAXI_RECONNECTED : SUBJ_REQUEST::NEW_TAXI);

    {
        lock_guard lock(this->mtx_producer);
        producer->produce(dotenv::getenv("TOPIC_REQUEST", "requests"), message);
    }

    return true;
}
