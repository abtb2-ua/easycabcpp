//
// Created by ab-flies on 28/11/24.
//

#include "Protocols.h"
#include <chrono>
#include <dotenv.h>
#include "Common.h"

using namespace prot;

////////////////////////////////////////////////////////////////////////////////
/// STORE AUXILIARY FUNCTIONS
////////////////////////////////////////////////////////////////////////////////
template<typename T>
void storeValue(vector<byte> &buffer, size_t &offset, const T *value, int n = -1) {
    if (n == -1)
        n = sizeof(*value);
    buffer.resize(buffer.size() + n);
    memcpy(buffer.data() + offset, value, n);
    offset += n;
}

template<typename T>
void storeValue(vector<byte> &buffer, size_t &offset, const T &value, int n = -1) {
    storeValue(buffer, offset, &value, n);
}

#define store(...) storeValue(buffer, offset, __VA_ARGS__)

////////////////////////////////////////////////////////////////////////////////
/// EXTRACT AUXILIARY FUNCTIONS
////////////////////////////////////////////////////////////////////////////////

template<typename T>
void extractValue(const span<const byte> &data, size_t &offset, T *value, int n = -1) {
    if (n == -1) {
        n = sizeof(*value);
    }
    memcpy(value, data.data() + offset, n);
    offset += n;
}

template<typename T>
void extractValue(const span<const byte> &data, size_t &offset, T &value, int n = -1) {
    extractValue(data, offset, &value, n);
}

void extractValue(const span<const byte> &data, size_t &offset, string &value, const int n) {
    value.resize(n);
    extractValue(data, offset, value.data(), n);
}

#define extract(...) extractValue(buffer, offset, __VA_ARGS__)


////////////////////////////////////////////////////////////////////////////////
/// CONSTRUCTORS
////////////////////////////////////////////////////////////////////////////////

Log::Log() {
    this->timestamp = getTimestamp();
    this->code = code_logs::MESSAGE::UNDEFINED;
    this->message = "";
    this->printBottom = false;
}

Log::Log(const code_logs::LogType code, const string &message, const bool printBottom) {
    this->timestamp = getTimestamp();
    this->code = code;
    this->message = message;
    this->printBottom = printBottom;
}

prot::Message::Message() {
    this->subject = SUBJ_RESPONSE::MAP_UPDATE;
    this->id = 0;
    this->taxiId = 0;
    this->coord = {0, 0};
    this->session = "";
    this->data.fill(byte{0});
}

////////////////////////////////////////////////////////////////////////////////
/// SERIALIZE AND DESERIALIZE FUNCTIONS
////////////////////////////////////////////////////////////////////////////////

vector<byte> Log::serialize() const {
    vector<byte> buffer;
    size_t offset = 0;

    store(this->printBottom);
    store(code_logs::toInt(this->code));
    store(this->timestamp.length());
    store(this->message.length());
    store(this->timestamp.data(), this->timestamp.length());
    store(this->message.data(), this->message.length());

    return buffer;
}

void Log::deserialize(const span<const byte>& buffer) {
    size_t offset = 0;
    size_t timestampLength, messageLength;
    int _code;

    extract(this->printBottom);
    extract(_code);
    this->code = code_logs::fromInt(_code);
    extract(timestampLength);
    extract(messageLength);
    extract(this->timestamp, timestampLength);
    extract(this->message, messageLength);
}

vector<byte> Map::serialize() const {
    vector<byte> buffer;
    size_t offset = 0;

    store(this->locations.size());
    store(this->customers.size());
    store(this->taxis.size());

    for (auto &location: locations) {
        store(location);
    }
    for (auto &customer: customers) {
        store(customer);
    }
    for (auto &taxi: taxis) {
        store(taxi);
    }

    return buffer;
}

void Map::deserialize(const span<const byte>& buffer) {
    size_t offset = 0;
    size_t locationsSize, customersSize, taxisSize;

    Location location;
    Customer customer;
    Taxi taxi;

    extract(locationsSize);
    extract(customersSize);
    extract(taxisSize);

    for (size_t i = 0; i < locationsSize; i++) {
        extract(location);
        this->locations.push_back(location);
    }

    for (size_t i = 0; i < customersSize; i++) {
        extract(customer);
        this->customers.push_back(customer);
    }

    for (size_t i = 0; i < taxisSize; i++) {
        extract(taxi);
        this->taxis.push_back(taxi);
    }
}

vector<byte> prot::Message::serialize() const {
    vector<byte> buffer;
    size_t offset = 0;

    store(this->subject);
    store(this->id);
    store(this->taxiId);
    store(this->coord);
    store(this->session.data(), this->session.size());
    store(this->data.data(), this->data.size());

    return buffer;
}

void prot::Message::deserialize(const span<const byte>& buffer) {
    size_t offset = 0;

    extract(this->subject);
    extract(this->id);
    extract(this->taxiId);
    extract(this->coord);
    extract(this->session, 36);
    extract(this->data.data(), this->data.size());
}

///////////////////////////////////////////////////////////////////////////
/// EXTENSION OF CPP_KAFKA CLASSES
///////////////////////////////////////////////////////////////////////////

Configuration prot::getConfig(const bool consumer, string id) {
    if (id.empty()) id = generate_unique_id();

    const string bootstrapServers = dotenv::getenv("KAFKA_BOOTSTRAP_SERVER", "127.0.0.1:9092");

    Configuration config;
    config.set("log_level", "5");
    config.set("bootstrap.servers", bootstrapServers);
    config.set("client.id", id);

    if (consumer) {
        config.set("session.timeout.ms", "6000");
        config.set("auto.offset.reset", "earliest");
        config.set("group.id", id);
        config.set("max.poll.interval.ms", "6000");
    } else {
        config.set("acks", "all");
    }

    if (envTrue("REDIRECT_KAFKA_LOGS")) {
        config.set_log_callback(kafka_file_logger);
    }

    return config;
}


