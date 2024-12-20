//
// Created by ab-flies on 28/11/24.
//

#ifndef KAFKA_SERVER_H
#define KAFKA_SERVER_H

#include <csignal>
#include "Logs.h"
#include "Protocols.h"

#include <MySQL/DbConnectionHandler.h>

using namespace std;
using namespace code_logs;
using namespace prot;

extern void centralLogHandler(ProducerWrapper &producer, const LogType &code, const string &message, bool printBottom);
extern string session;

class KafkaServer {
    unique_ptr<ProducerWrapper> producer;
    unique_ptr<ConsumerWrapper> consumer;
    unique_ptr<DbConnectionHandler> db;

    prot::Message response;

    void initProducer() { if (producer == nullptr) producer = make_unique<ProducerWrapper>("central-kafka-producer"); }
    void initConsumer() { if (consumer == nullptr) consumer = make_unique<ConsumerWrapper>("central-kafka-consumer"); }
    void initDb() {
        if (db == nullptr) {
            db = make_unique<DbConnectionHandler>();
            db->connect();
        }
    }

    void startInternal();
    KafkaServer() = default;

    void refreshMap() const;

    optional<short> checkAvailableTaxis() const;
    optional<char> checkQueue() const;
    void enqueue(char customerId) const;
    void prioritize(char customerId) const;
    void assign(short taxiId, char customerId);
    bool setDestination(char customerId, char service) const;
    optional<char> carryingClient(short taxiId) const;
    optional<char> getService(short taxiId) const;
    optional<Coordinate> getDestination(char customerId) const;
    void setOnboard(char customerId, short taxiId, bool val) const;

    // Actions
    void newTaxi(const prot::Message &request);
    void newCustomer(const prot::Message &request);
    void taxiReconnected(const prot::Message &request);

    static void checkStrays();

    //////////////////////////////
    /// ENTRY POINT
    //////////////////////////////

    static KafkaServer server;

public:
    static void start() {
        server.startInternal();
    }
};

#endif // KAFKA_SERVER_H
