//
// Created by ab-flies on 18/11/24.
//

#ifndef PIPE_HANDLER_H
#define PIPE_HANDLER_H
#include <Logs.h>
#include <SocketHandler.h>

using namespace code_logs;

class PipeHandler {
    int pipefd[2];

public:
    static constexpr int BUFFER_SIZE = SocketHandler::BUFFER_SIZE;

    class PipeReader;
    class PipeWriter;

    // Canonical form
    PipeHandler();

    PipeHandler(const PipeHandler &other) = delete;

    PipeHandler(PipeHandler &&other) = delete;

    PipeHandler &operator=(const PipeHandler &other) = delete;

    PipeHandler &operator=(PipeHandler &&other) = delete;

    // Methods
    [[nodiscard]] PipeReader getReader() const;

    [[nodiscard]] PipeWriter getWriter() const;
};


class PipeHandler::PipeReader {
    friend class PipeHandler;

    constexpr static short TM_UNSET = -1;

    SocketHandler pipeHandler;
    int tm_seconds, tm_microseconds;
    int pipe;

    explicit PipeReader(int pipe);

public:
    // Canonical form
    PipeReader(const PipeReader &other);

    PipeReader(PipeReader &&other) noexcept;

    PipeReader &operator=(const PipeReader &other) = delete;

    PipeReader &operator=(PipeReader &&other) noexcept;

    READ_RETURN_CODE readFromPipe() { return pipeHandler.readFromSocket(); }

    [[nodiscard]] bool hasData() const { return pipeHandler.hasData(); }

    [[nodiscard]] bool hasControlChar() const { return pipeHandler.hasControlChar(); }

    [[nodiscard]] bool lastReadFailed() const { return pipeHandler.lastReadFailed(); }

    [[nodiscard]] span<const byte> getData() const { return pipeHandler.getData(); }

    [[nodiscard]] CONTROL_CHAR getControlChar() const { return pipeHandler.getControlChar(); }

    ~PipeReader() = default;

    // Methods
    void setTimeout(const int seconds, const int microseconds = 0) {
        tm_seconds = seconds;
        tm_microseconds = microseconds;
    };

    void unsetTimeout() {
        tm_seconds = tm_microseconds = TM_UNSET;
    }

    [[nodiscard]] READ_RETURN_CODE readFromPipe();
};

class PipeHandler::PipeWriter {
    friend class PipeHandler;

    SocketHandler pipeHandler;
    int pipe;

    explicit PipeWriter(int pipe);

public:
    // Canonical form
    PipeWriter(const PipeWriter &other) = delete;

    PipeWriter(PipeWriter &&other) noexcept;

    PipeWriter &operator=(const PipeWriter &other) = delete;

    PipeWriter &operator=(PipeWriter &&other) noexcept;

    // Methods
    bool writeToPipe() { return pipeHandler.writeToSocket(); }

    bool sendControlChar(const CONTROL_CHAR ch) { return pipeHandler.sendControlChar(ch); }

    span<byte> getBuffer() { return pipeHandler.getBuffer(); }
};

#endif //PIPE_HANDLER_H
