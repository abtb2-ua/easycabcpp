//
// Created by ab-flies on 18/11/24.
//

#include "PipeHandler.h"

#include <unistd.h>
#include <stdexcept>

using namespace std;

PipeHandler::PipeHandler(): pipefd() {
    if (pipe(this->pipefd) == -1) {
        throw runtime_error("Error creating pipe");
    }
}

PipeHandler::PipeReader PipeHandler::getReader() const {
    return PipeReader(this->pipefd[0]);
}

PipeHandler::PipeWriter PipeHandler::getWriter() const {
    return PipeWriter(this->pipefd[1]);
}

PipeHandler::PipeReader::PipeReader(const int pipe): pipeHandler(pipe) {
    this->pipe = pipe;
    this->pipeHandler.setCloseAtEnd(false);
    this->tm_microseconds = TM_UNSET;
    this->tm_seconds = TM_UNSET;
}

PipeHandler::PipeReader::PipeReader(const PipeReader &other) {
    this->pipe = other.pipe;
    this->pipeHandler = move(SocketHandler(other.pipe));
    this->pipeHandler.setCloseAtEnd(false);
    this->tm_microseconds = other.tm_microseconds;
    this->tm_seconds = other.tm_seconds;
}

PipeHandler::PipeReader::PipeReader(PipeReader &&other) noexcept {
    this->pipe = other.pipe;
    this->pipeHandler = move(other.pipeHandler);
    this->tm_microseconds = other.tm_microseconds;
    this->tm_seconds = other.tm_seconds;
}

PipeHandler::PipeReader &PipeHandler::PipeReader::operator=(PipeReader &&other) noexcept {
    if (this == &other) { return *this; }

    pipe = other.pipe;
    pipeHandler = move(other.pipeHandler);
    tm_microseconds = other.tm_microseconds;
    tm_seconds = other.tm_seconds;

    return *this;
}

READ_RETURN_CODE PipeHandler::PipeReader::readFromPipe() {
    fd_set set;
    FD_ZERO(&set);
    FD_SET(pipe, &set);

    if (this->tm_microseconds != TM_UNSET && this->tm_seconds != TM_UNSET) {
        timeval timeout = {this->tm_seconds, this->tm_microseconds};

        const int ret = select(pipe + 1, &set, nullptr, nullptr, &timeout);
        if (ret == 0) {
            return READ_RETURN_CODE::TIMEOUT;
        }
        if (ret < 0) {
            return READ_RETURN_CODE::FAILED;
        }
    }

    return pipeHandler.readFromSocket();
}

PipeHandler::PipeWriter::PipeWriter(const int pipe): pipeHandler(pipe) {
    this->pipe = pipe;
    this->pipeHandler.setCloseAtEnd(false);
}

PipeHandler::PipeWriter::PipeWriter(PipeWriter &&other) noexcept {
    this->pipe = other.pipe;
    this->pipeHandler = move(other.pipeHandler);
}

PipeHandler::PipeWriter &PipeHandler::PipeWriter::operator=(PipeWriter &&other) noexcept {
    if (this == &other) { return *this; }

    pipe = other.pipe;
    pipeHandler = move(other.pipeHandler);

    return *this;
}
