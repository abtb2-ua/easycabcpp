//
// Created by ab-flies on 18/11/24.
//

#include "Logs.h"
#include "Common.h"
#include <chrono>
#include <iomanip>
#include <ctime>
#include <vector>

namespace code_logs {
    map<LogType, string> formats = {
        {MESSAGE::DEBUG, "{}"},
        {MESSAGE::UNDEFINED, "{}"},
        {ERROR::UNDEFINED, "{}"},
        {WARNING::UNDEFINED, "a ver {} hola"},

        {MESSAGE::LOADED_LOC_FILE, "Loaded locations file"},
    };


    string to_string(const LogType &code) {
        if (holds_alternative<ERROR>(code)) {
            return "** ERROR **";
        }
        if (holds_alternative<WARNING>(code)) {
            return "** WARNING **";
        }

        const auto message = get<MESSAGE>(code);
        if (message == MESSAGE::DEBUG) {
            return "Debug";
        }

        return "Message";
    }

    string getCodeANSIColor(const LogType &code) {
        const string GREEN = "\x1b[38;5;156m";
        const string PINK = "\x1b[38;5;219m";
        const string BOLD = "\x1b[1m";
        string ORANGE = "\x1b[38;5;215m";
        string RED = "\x1b[38;5;9m";

        if (holds_alternative<ERROR>(code)) {
            return RED;
        }
        if (holds_alternative<WARNING>(code)) {
            return ORANGE;
        }

        const auto message = get<MESSAGE>(code);
        if (message == MESSAGE::DEBUG) {
            return BOLD + PINK;
        }

        return BOLD + GREEN;
    }

    void defaultLogHandler(const LogType &code, const string &message) {
        const string RESET = "\x1b[0m";
        const string BLUE = "\x1b[38;5;75m";

        using namespace chrono;

        const auto now = system_clock::now();
        const time_t current_time = system_clock::to_time_t(now);
        const tm local_time = *localtime(&current_time);

        auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
        cout << BLUE << put_time(&local_time, "[%H:%M:%S") << format(".{:03}] ", ms.count()) << RESET
                << getCodeANSIColor(code) << to_string(code) << RESET
                << ": " << message << endl;

        if (holds_alternative<ERROR>(code)) {
            exit(1);
        }
    }
}
