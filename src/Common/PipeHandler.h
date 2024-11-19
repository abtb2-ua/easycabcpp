//
// Created by ab-flies on 18/11/24.
//

#ifndef PIPE_HANDLER_H
#define PIPE_HANDLER_H
#include <SocketHandler.h>


class PipeHandler {
    int pipefd[2];

public:
    // Canonical form
    PipeHandler();

    PipeHandler(const PipeHandler &other) = delete;

    PipeHandler(PipeHandler &&other) = delete;

    PipeHandler &operator=(const PipeHandler &other) = delete;

    PipeHandler &operator=(PipeHandler &&other) = delete;

    ~PipeHandler() = default;

    // Nested classes
    class PipeReader;
    class PipeWriter;

    // Methods
    PipeReader getReader() const;
    PipeWriter getWriter() const;
};

class PipeHandler::PipeReader {
    friend class PipeHandler;

    SocketHandler pipeHandler;
    int pipe;

    explicit PipeReader(int pipe);

public:

    // Canonical form
    PipeReader(const PipeReader &other);

    PipeReader(PipeReader &&other) = delete;

    PipeReader &operator=(const PipeReader &other);

    PipeReader &operator=(PipeReader &&other) = delete;

    ~PipeReader() = default;

    // Methods
    bool setTimeout(const int seconds, const int microseconds = 0) const { // NOLINT(*-use-nodiscard)
        return pipeHandler.setTimeout(seconds, microseconds);
    };

    SocketHandler::READ_RETURN_CODE readFromPipe(char buffer[SocketHandler::BUFFER_SIZE]) const {
        return pipeHandler.readFromSocket(buffer);
    };
};


class PipeHandler::PipeWriter {
    friend class PipeHandler;

    SocketHandler pipeHandler;
    int pipe;

    explicit PipeWriter(int pipe);

public:
    // Canonical form
    PipeWriter(const PipeWriter &other);

    PipeWriter(PipeWriter &&other) = delete;

    PipeWriter &operator=(const PipeWriter &other);

    PipeWriter &operator=(PipeWriter &&other) = delete;

    ~PipeWriter() = default;

    // Methods
    bool writeToPipe(const char buffer[SocketHandler::BUFFER_SIZE]) const {
        return pipeHandler.writeToSocket(buffer);
    };

    bool sendCode(const SocketHandler::CODE code) const { // NOLINT(*-use-nodiscard)
        return pipeHandler.sendCode(code);
    };
};

#endif //PIPE_HANDLER_H
