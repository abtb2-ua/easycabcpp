#include "Common.h"
#include <glib.h>
#include <cstdlib>
#include <sys/time.h>
#include <string>
#include <sstream>
#include <fstream>
#include <cppkafka/cppkafka.h>
#include <dotenv.h>

using namespace std;


Address::Address() {
    this->ip = "";
    this->port = 0;
}

Address::Address(const string &address) {
    const size_t pos = address.find(':');
    if (pos == string::npos) {
        this->ip = "";
        this->port = 0;
        return;
    }

    this->ip = address.substr(0, pos);
    this->port = stoi(address.substr(pos + 1));
}

Address::Address(const string &ip, const int port) {
    if (!this->setIp(ip) || !this->setPort(port)) {
        this->ip = "";
        this->port = 0;
    }
}

bool Address::setIp(const string &ip) {
    if (ip.empty()) {
        return false;
    }

    int n;
    char c;
    stringstream ss(ip);
    for (int i = 0; i < 4; i++) {
        ss >> n;
        if (ss.fail() || n < 0 || n > 255) {
            return false;
        }

        if (i < 3) {
            ss >> c;
            if (c != '.') {
                return false;
            }
        }
    }

    this->ip = ip;
    return true;
}

bool Address::setPort(const int port) {
    if (port < 0 || port > 65535) {
        return false;
    }

    this->port = port;
    return true;
}

bool envTrue(const char* _env) {
    const char *env = getenv(_env);

    if (!env) {
        return false;
    }

    string lower = env;
    ranges::transform(lower, lower.begin(), ::tolower);
    return lower == "true" || lower == "1" || lower == "yes" || lower == "y" || lower == "on";
}

void log_handler(const gchar *log_domain, const GLogLevelFlags log_level, const gchar *message,
                 gpointer user_data) {
    const auto GREEN = "\x1b[38;5;156m";
    const auto RESET = "\x1b[0m";
    const auto BOLD = "\x1b[1m";
    const auto BLUE = "\x1b[38;5;75m";
    const auto ORANGE = "\x1b[38;5;215m";
    const auto PURPLE = "\x1b[38;5;135m";
    const auto RED = "\x1b[38;5;9m";
    const auto PINK = "\x1b[38;5;219m";

    if ((log_level & G_LOG_LEVEL_MASK) == G_LOG_LEVEL_DEBUG
        && !debugging()
        && !envTrue("DEBUG")) {
        return;
    }

    struct timeval now = {};
    gettimeofday(&now, nullptr);
    const int hours = static_cast<int>(now.tv_sec / 3600 % 24) + 2; // UTC + 2
    const int minutes = static_cast<int>(now.tv_sec / 60 % 60);
    const int seconds = static_cast<int>(now.tv_sec) % 60;
    const int milliseconds = static_cast<int>(now.tv_usec) / 1000;

    printf("%s[%02i:%02i:%02i.%03i]%s ", BLUE, hours, minutes, seconds, milliseconds, RESET);

    switch (log_level & G_LOG_LEVEL_MASK) {
        case G_LOG_LEVEL_CRITICAL:
            printf("%s** CRITICAL **%s", PURPLE, RESET);
            break;
        case G_LOG_LEVEL_WARNING:
            printf("%s** WARNING **%s", ORANGE, RESET);
            break;
        case G_LOG_LEVEL_MESSAGE:
            printf("%s%sMessage%s", GREEN, BOLD, RESET);
            break;
        case G_LOG_LEVEL_DEBUG:
            printf("%s%sDebug%s", PINK, BOLD, RESET);
            break;
        case G_LOG_LEVEL_INFO:
            printf("%s%sInfo%s", BLUE, BOLD, RESET);
            break;
        case G_LOG_LEVEL_ERROR:
            printf("%s** ERROR **%s", RED, RESET);
            break;

        default:
            break;
    }

    printf(": %s\n", message);

    if (log_level == 6 || log_level == G_LOG_LEVEL_ERROR) {
        exit(1);
    }
}

string getKafkaLogsPath(const cppkafka::KafkaHandleBase &handle) {
    return dotenv::getenv("KAFKA_LOG_PATH", "kafka_logs/") + "/" + handle.get_name() + ".log";
}

void kafka_file_logger(const cppkafka::KafkaHandleBase &handle,
              int level,
              const string &facility,
              const string &message) {
    const string path = dotenv::getenv("KAFKA_LOG_PATH", "kafka_logs") + "/" + handle.get_name() + ".log";
    ofstream output_file(path, ios_base::app);

    if (!output_file.is_open()) {
        return;
    }

    output_file << message;

    output_file.close();
}

void kafka_glib_logger(const cppkafka::KafkaHandleBase &handle,
              int level,
              const string &facility,
              const string &message) {
    g_info("[%s]: %s", handle.get_name().c_str(), message.c_str());
}


cppkafka::Configuration getConfig(const bool consumer, string id) {
    if (id.empty()) {
        array<char, UUID_LENGTH> buffer = {};
        generate_unique_id(buffer);
        id = buffer.data();
    }

    const string bootstrapServers = dotenv::getenv("KAFKA_BOOTSTRAP_SERVER", "127.0.0.1:9092");

    cppkafka::Configuration config;
    config.set("log_level", "5");
    config.set("bootstrap.servers", bootstrapServers);
    config.set("client.id", id);
    config.set("acks", "all");

    if (consumer) {
        config.set("session.timeout.ms", "6000");
        config.set("auto.offset.reset", "earliest");
        config.set("group.id", id);
        config.set("max.poll.interval.ms", "6000");
    }

    if (envTrue("REDIRECT_KAFKA_LOGS")) {
        config.set_log_callback(kafka_file_logger);
    } else {
        config.set_log_callback(kafka_glib_logger);
    }

    return config;
}

vector<string> splitLines(const string &str, const size_t lineLength) {
    vector<string> lines;
    istringstream ss(str);
    string word;

    while (getline(ss, word, ' ')) {
        if (lines.empty() || lines.back().size() + word.size() + 1 > lineLength) {
            lines.push_back(word);
        } else {
            lines.back() += " " + word;
        }
    }

    return lines;
}