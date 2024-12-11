//
// Created by ab-flies on 28/11/24.
//

#ifndef AUTH_SERVER_H
#define AUTH_SERVER_H

#include <csignal>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <unistd.h>
#include "Logs.h"
#include "Protocols.h"
#include "Socket/SocketHandler.h"

#include <MySQL/DbConnectionHandler.h>

using namespace std;
using namespace code_logs;
using namespace prot;

extern void centralLogHandler(ProducerWrapper &producer, const LogType &code, const string &message, bool printBottom);

extern string session;

using namespace std;

class AuthServer {
    unique_ptr<ProducerWrapper> producer = nullptr;
    DbConnectionHandler db;
    mutex mtx_producer, mtx_db;

    [[noreturn]] void listenPetitions();
    void initProducer() { if (producer == nullptr) producer = make_unique<ProducerWrapper>(); }
    void attendPetition(SocketHandler &&client, int petitionId);
    bool checkId(const string& author, short taxiId, bool& reconnect);
    bool addTaxi(const string& author, short taxiId, bool reconnect);
    AuthServer() = default;

    //////////////////////////////
    /// ENTRY POINT
    //////////////////////////////

    static AuthServer server;

public:
    static void start() { server.listenPetitions(); }
};

// AuthServer AuthServer::server;
#endif // AUTH_SERVER_H
