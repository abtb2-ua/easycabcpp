//
// Created by ab-flies on 18/11/24.
//

#ifndef LOGS_H
#define LOGS_H

#include <variant>
#include <iostream>
#include <fmt/core.h>
#include <map>
#include "Common.h"

using namespace std;

namespace code_logs {
    enum class MESSAGE {
        UNDEFINED,
        DEBUG,
        LOADED_LOC_FILE,
    };

    enum class WARNING {
        UNDEFINED,
    };

    enum class ERROR {
        UNDEFINED,
        CREATING_PROCESS,
    };

    using LogType = variant<MESSAGE, ERROR, WARNING>;

    extern map<LogType, string> formats;

    string to_string(const LogType &code);

    void defaultLogHandler(const LogType &code, const string &message);

    inline function logHandler = defaultLogHandler;

    void setDefaultLogHandler(const auto &handler) { logHandler = handler; }


    template<typename T, size_t... Indices>
    auto vectorToTupleHelper(const vector<T> &v, index_sequence<Indices...>) {
        return make_tuple(v[Indices]...);
    }

    template<size_t N, typename T>
    auto vectorToTuple(const vector<T> &v) {
        return vectorToTupleHelper(v, make_index_sequence<N>());
    }

    // Higher number of arguments a format string has.
    // Any lacking argument will be replaced by an empty string.
    constexpr int NUM_ARGS = 4;

    // Variadic template function that only accepts strings (except for the first argument)
    template<typename... Args>
    concept AllStrings = (is_convertible_v<Args, string> && ...);

    void codeLog(const LogType &code, AllStrings auto... args) {
        string format_str = formats.contains(code) ? formats[code] : formats[MESSAGE::UNDEFINED];

        // Convert the arguments to a vector of strings to be able to resize it
        vector<string> args_vec = {(args)...};
        args_vec.resize(NUM_ARGS);

        // Convert the vector to a tuple to be able to unpack it
        auto args_tuple = vectorToTuple<NUM_ARGS>(args_vec);

        // Call fmt::format with sanitized arguments
        const string message = apply(
            [format_str](auto &&... unpackedArgs) { return fmt::format(fmt::runtime(format_str), unpackedArgs...); },
            args_tuple
        );

        logHandler(code, message);
    }
}

#endif //LOGS_H
