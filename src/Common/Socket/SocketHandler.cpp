#include "Socket/SocketHandler.h"
#include <arpa/inet.h>
#include <cstdio>
#include <glib.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include "Common.h"
#include <magic_enum/magic_enum.hpp>

using namespace std;
using namespace code_logs;

// Canonical form
SocketHandler::SocketHandler() : socketfd(CLOSED), buffer({}), data({}) {}

SocketHandler::SocketHandler(const int s) : socketfd(s), buffer({}), data({}) {}

SocketHandler::SocketHandler(SocketHandler &&other) noexcept {
    this->socketfd = other.socketfd;
    this->closeAtEnd = other.closeAtEnd;
    this->data = move(other.data);
    this->buffer = move(other.buffer);
    this->error = other.error;

    other.socketfd = CLOSED;
}

SocketHandler &SocketHandler::operator=(SocketHandler &&other) noexcept {
    if (this == &other)
        return *this;

    if (this->socketfd != CLOSED && this->closeAtEnd) {
        close(this->socketfd);
    }

    this->socketfd = other.socketfd;
    this->closeAtEnd = other.closeAtEnd;
    this->data = move(other.data);
    this->buffer = move(other.buffer);
    this->error = other.error;

    other.socketfd = CLOSED;

    return *this;
}

SocketHandler::~SocketHandler() {
    if (this->closeAtEnd) {
        this->closeSocket();
    }
}

bool SocketHandler::openSocket(const int port) {
    constexpr int opt = 1;
    this->socketfd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in server = {};

    if (!this->isOpen())
        return false;

    if (setsockopt(this->socketfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        this->closeSocket();
        return false;
    }

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = INADDR_ANY;
    if (bind(this->socketfd, reinterpret_cast<sockaddr *>(&server), sizeof(server)) == -1) {
        this->closeSocket();
        return false;
    }


    if (listen(this->socketfd, 4) == -1) {
        this->closeSocket();
        return false;
    }

    return true;
}

bool SocketHandler::connectTo(const Address &serverAddress) {
    sockaddr_in server = {};
    this->socketfd = socket(AF_INET, SOCK_STREAM, 0);

    if (!this->isOpen())
        return false;

    server.sin_addr.s_addr = inet_addr(serverAddress.getIp().c_str());
    server.sin_family = AF_INET;
    server.sin_port = htons(serverAddress.getPort());

    if (connect(this->socketfd, reinterpret_cast<struct sockaddr *>(&server), sizeof(server)) == -1) {
        this->closeSocket();
        return false;
    }

    return true;
}

void SocketHandler::closeSocket() {
    if (this->socketfd != CLOSED) {
        close(this->socketfd);
        this->socketfd = CLOSED;
    }
}

bool SocketHandler::isOpen() const { return this->socketfd != CLOSED; }

SocketHandler SocketHandler::acceptConnections() const {
    if (this->socketfd == CLOSED) {
        return {};
    }

    SocketHandler client(accept(this->socketfd, nullptr, nullptr));

    if (!client.isOpen())
        return {};

    return client;
}

bool SocketHandler::setTimeout(const int seconds, const int microseconds) const {
    if (this->socketfd == CLOSED) {
        return false;
    }

    timeval timeout{};
    timeout.tv_sec = seconds;
    timeout.tv_usec = microseconds;

    if (setsockopt(this->socketfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        return false;
    }

    return true;
}

bool SocketHandler::checkDataFormat() {
    const auto code = static_cast<CONTROL_CHAR>(data.front());

    if (code == CONTROL_CHAR::STC) {
        // Check if the control char is valid
        // Control char enum shouldn't change in the future, so it's relatively safe to hardcode the values
        return magic_enum::enum_contains<CONTROL_CHAR>(static_cast<short>(data[1]));
        return static_cast<uint8_t>(data[1]) <= static_cast<uint8_t>(CONTROL_CHAR::NONE);
    }

    if (code == CONTROL_CHAR::STX) {
        return data[data.size() - 2] == static_cast<byte>(CONTROL_CHAR::ETX);
    }

    return false;
}

bool SocketHandler::readFromSocketInternal() {
    if (this->socketfd == CLOSED) {
        error = READ_ERROR::SOCKET_CLOSED;
        return false;
    }


    if (read(this->socketfd, this->data.data(), BUFFER_SIZE + 3) < 1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            error = READ_ERROR::TIMEOUT;
        } else {
            error = READ_ERROR::READ_FAILED;
        }
        return false;
    }

    if (!this->checkDataFormat()) {
        error = READ_ERROR::INVALID_FORMAT;
        return false;
    }

    if (data.front() == static_cast<byte>(CONTROL_CHAR::STX)) {
        byte lrc{0};
        for (const auto it: span{data.begin(), data.end() - 1}) {
            lrc ^= it;
        }

        if (lrc != data.back()) {
            error = READ_ERROR::INVALID_LRC;
            return false;
        }
    }

    error = READ_ERROR::NONE;
    return true;
}

bool SocketHandler::writeToSocketInternal() {
    if (this->socketfd == CLOSED) {
        return false;
    }

    byte lrc{0};
    for (const auto it: span{buffer.begin(), buffer.end() - 1}) {
        lrc ^= it;
    }

    this->buffer.back() = lrc;

    return (write(this->socketfd, buffer.data(), BUFFER_SIZE + 3) > 0);
}

bool SocketHandler::writeToSocket() {
    this->buffer[0] = static_cast<byte>(CONTROL_CHAR::STX);
    *(this->buffer.rbegin() + 1) = static_cast<byte>(CONTROL_CHAR::ETX);
    return this->writeToSocketInternal();
}

bool SocketHandler::sendControlChar(CONTROL_CHAR ch) {
    this->buffer[0] = static_cast<byte>(CONTROL_CHAR::STC);
    this->buffer[1] = static_cast<byte>(ch);
    return this->writeToSocketInternal();
}

bool SocketHandler::readFromSocket() {
    const bool ret = this->readFromSocketInternal();
    if (this->getError() != READ_ERROR::NONE) {
        this->data[0] = static_cast<byte>(CONTROL_CHAR::NONE);
    }
    return ret;
}

span<const byte> SocketHandler::getData() const {
    if (!this->hasData()) {
        array<byte, BUFFER_SIZE> empty = {};
        return {empty.begin(), empty.end()};
    }
    return {this->data.begin() + 1, BUFFER_SIZE};
}

CONTROL_CHAR SocketHandler::getControlChar() const {
    if (!this->hasControlChar())
        return CONTROL_CHAR::NONE;
    return static_cast<CONTROL_CHAR>(this->data[1]);
}
