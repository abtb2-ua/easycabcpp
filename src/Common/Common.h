#ifndef COMMON_H
#define COMMON_H

#include <glib.h>
#include <cppkafka/cppkafka.h>
#include <iostream>
#include <uuid/uuid.h>
#include <functional>

#ifdef __linux__
#include <sys/ptrace.h>
#endif

// Includes the null terminator
#define UUID_LENGTH 37

using namespace std;

inline bool debugging() {
#ifdef __linux__
    return ptrace(PTRACE_TRACEME, 0, nullptr, 0) == -1;
#elif _WIN32
    return IsDebuggerPresent();
#else
    return false;
#endif
}

class Address {
    string ip;
    int port;

public:
    Address();

    explicit Address(const string &address);

    Address(const string &ip, int port);

    [[nodiscard]] string getIp() const { return this->ip; }

    [[nodiscard]] int getPort() const { return this->port; }

    bool setIp(const string &ip);

    bool setPort(int port);

    [[nodiscard]] string toString() const { return this->ip + ":" + to_string(this->port); }
};

bool envTrue(const char* _env);

void log_handler(const gchar *log_domain, GLogLevelFlags log_level, const gchar *message,
                 gpointer user_data);

void kafka_file_logger(const cppkafka::KafkaHandleBase &handle,
                       int level,
                       const string &facility,
                       const string &message);

inline void generate_unique_id(char uuid[UUID_LENGTH]) {
    uuid_t bin_uuid;
    uuid_generate_random(bin_uuid);
    uuid_unparse(bin_uuid, uuid);
}

string getKafkaLogsPath(const cppkafka::KafkaHandleBase &handle);

cppkafka::Configuration getConfig(bool consumer, string id = "");

bool createProcess(const function<void(const void*)> &callback, const void* args);

vector<string> splitLines(const string &str, size_t lineLength);


#endif //COMMON_H
