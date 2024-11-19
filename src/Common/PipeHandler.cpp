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

PipeHandler::PipeReader::PipeReader(const int pipe): pipeHandler(pipe), pipe(pipe) {
    pipeHandler.setCloseAtEnd(false);
}

PipeHandler::PipeReader::PipeReader(const PipeReader &other): pipeHandler(other.pipe), pipe(other.pipe) {
    pipeHandler.setCloseAtEnd(false);
}

PipeHandler::PipeReader &PipeHandler::PipeReader::operator=(const PipeReader &other) {
    if (this == &other) { return *this; }

    pipeHandler = move(SocketHandler(other.pipe));
    pipe = other.pipe;

    pipeHandler.setCloseAtEnd(false);

    return *this;
}

PipeHandler::PipeWriter::PipeWriter(const int pipe): pipeHandler(pipe), pipe(pipe) {
    pipeHandler.setCloseAtEnd(false);
}

PipeHandler::PipeWriter::PipeWriter(const PipeWriter &other): pipeHandler(other.pipe), pipe(other.pipe) {
    pipeHandler.setCloseAtEnd(false);
}

PipeHandler::PipeWriter &PipeHandler::PipeWriter::operator=(const PipeWriter &other) {
    if (this == &other) { return *this; }

    pipeHandler = move(SocketHandler(other.pipe));
    pipe = other.pipe;

    pipeHandler.setCloseAtEnd(false);

    return *this;
}


