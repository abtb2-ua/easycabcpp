//
// Created by ab-flies on 28/11/24.
//

#ifndef KAFKA_SERVER_H
#define KAFKA_SERVER_H

#include <csignal>
#include "Logs.h"
#include "Protocols.h"

using namespace std;
using namespace code_logs;
using namespace prot;

extern void centralLogHandler(ProducerWrapper &producer, const LogType &code, const string &message, bool printBottom);

class KafkaServer {
    void startInternal() { while (true) { sleep(1); }}
    KafkaServer() = default;


    //////////////////////////////
    /// ENTRY POINT
    //////////////////////////////

    static KafkaServer server;

public:
    static void start() { server.startInternal(); }
};

#endif // KAFKA_SERVER_H
