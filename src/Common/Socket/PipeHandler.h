//
// Created by ab-flies on 18/11/24.
//

#ifndef PIPE_HANDLER_H
#define PIPE_HANDLER_H
#include "Logs.h"
#include "Socket/SocketHandler.h"
#include "Protocols.h"

using namespace code_logs;

class PipeHandler {
    int pipefd[2];

public:
    static constexpr int BUFFER_SIZE = SocketHandler::BUFFER_SIZE;

    class Reader;
    class Writer;

    // Canonical form
    PipeHandler();

    PipeHandler(const PipeHandler &other) = delete;

    PipeHandler(PipeHandler &&other) = delete;

    PipeHandler &operator=(const PipeHandler &other) = delete;

    PipeHandler &operator=(PipeHandler &&other) = delete;

    // Methods
    [[nodiscard]] Reader getReader() const;

    [[nodiscard]] Writer getWriter() const;
};


class PipeHandler::Reader {
    friend class PipeHandler;

    constexpr static short TM_UNSET = -1;

    SocketHandler pipeHandler;
    int tm_seconds, tm_microseconds;
    int pipe;

    explicit Reader(int pipe);

public:
    // Canonical form
    Reader(const Reader &other);

    Reader(Reader &&other) noexcept;

    Reader &operator=(const Reader &other) = delete;

    Reader &operator=(Reader &&other) noexcept;

    ~Reader() = default;

    // Methods
    // READ_RETURN_CODE readFromPipe();

    [[nodiscard]] bool hasData() const { return pipeHandler.hasData(); }

    [[nodiscard]] bool hasControlChar() const { return pipeHandler.hasControlChar(); }

    // [[nodiscard]] bool lastReadFailed() const { return pipeHandler.lastReadFailed(); }

    [[nodiscard]] span<const byte> getData() const { return pipeHandler.getData(); }

    [[nodiscard]] CONTROL_CHAR getControlChar() const { return pipeHandler.getControlChar(); }
    void setTimeout(const int seconds, const int microseconds = 0) {
        tm_seconds = seconds;
        tm_microseconds = microseconds;
    };

    void unsetTimeout() {
        tm_seconds = tm_microseconds = TM_UNSET;
    }
};

class PipeHandler::Writer {
    friend class PipeHandler;

    SocketHandler pipeHandler;
    int pipe;

    explicit Writer(int pipe);

public:
    // Canonical form
    Writer(const Writer &other) = delete;

    Writer(Writer &&other) noexcept;

    Writer &operator=(const Writer &other) = delete;

    Writer &operator=(Writer &&other) noexcept;

    // Methods
    bool writeToPipe() { return pipeHandler.writeToSocket(); }

    bool sendControlChar(const CONTROL_CHAR ch) { return pipeHandler.sendControlChar(ch); }

    span<byte> getBuffer() { return pipeHandler.getBuffer(); }
};

#endif //PIPE_HANDLER_H
