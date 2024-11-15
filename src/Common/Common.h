#ifndef COMMON_H
#define COMMON_H

#include <glib.h>
#include <cppkafka/cppkafka.h>
#include <iostream>
#include <uuid/uuid.h>

#ifdef __linux__
#include <sys/ptrace.h>
#endif

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
private:
    string ip;
    int port;

public:
    Address();

    explicit Address(const string &address);

    Address(const string &ip, int port);

    [[nodiscard]] string getIp() const;

    [[nodiscard]] int getPort() const;

    bool setIp(const string &ip);

    bool setPort(int port);

    [[nodiscard]] string toString() const;
};

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

#endif //COMMON_H
