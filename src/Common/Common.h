#ifndef COMMON_H
#define COMMON_H

#include <any>
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

inline void generate_unique_id(array<char, UUID_LENGTH>& uuid) {
    uuid_t bin_uuid;
    uuid_generate_random(bin_uuid);
    uuid_unparse(bin_uuid, uuid.data());
}

string getKafkaLogsPath(const cppkafka::KafkaHandleBase &handle);

cppkafka::Configuration getConfig(bool consumer, string id = "");


#include <functional>
#include <utility>
#include <sys/types.h>
#include <unistd.h>
#include <cstdlib>

template <typename Func, typename... Args>
bool createProcess(Func&& callback, Args&&... args) {
    const pid_t pid = fork();

    if (pid == -1) {
        return false; // Fork failed
    }

    // If child process
    if (pid == 0) {
        invoke(forward<Func>(callback), forward<Args>(args)...);
        exit(0);
    }

    return true;
}


vector<string> splitLines(const string &str, size_t lineLength);


#endif //COMMON_H
