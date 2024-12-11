#ifndef COMMON_H
#define COMMON_H

#include "Logs.h"
#include <any>
#include <cppkafka/cppkafka.h>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <utility>
#include <uuid.h>

using namespace std;

#ifdef __linux__
#include <sys/ptrace.h>
#endif

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



// Includes the null terminator
string generate_unique_id();

// [hh:mm:SS.sss]_: 15 chars + '\0'
string getTimestamp();

bool envTrue(const string& env);

int envInt(const string& env, int defaultValue = 0);

void kafka_file_logger(const cppkafka::KafkaHandleBase &handle, int level, const string &facility,
                       const string &message);

string getKafkaLogsPath(const cppkafka::KafkaHandleBase &handle);

template<typename Func, typename... Args>
pid_t createProcess(Func &&callback, Args &&...args) {
    const pid_t pid = fork();

    if (pid == -1) {
        codeLog(code_logs::ERROR::CREATING_PROCESS);
        exit(1);
    }

    // If child process
    if (pid == 0) {
        invoke(forward<Func>(callback), forward<Args>(args)...);
        exit(0);
    }

    return pid;
}


vector<string> splitLines(const string &str, size_t lineLength);

inline string centerString(const string &str, size_t width) {
    return format("{:^{}}", str, width);
}

#endif // COMMON_H
