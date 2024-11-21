#include "SocketHandler.h"
#include "Common.h"
#include <cstdio>
#include <unistd.h>
#include <glib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

using namespace std;


// Canonical form

SocketHandler::~SocketHandler() {
    if (this->closeAtEnd) {
        this->closeSocket();
    }
}

bool SocketHandler::openSocket(const int port) {
    constexpr int opt = 1;
    this->socketfd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in server = {};

    if (!this->isOpen()) {
        g_warning("Error opening socket.");
        return false;
    }

    if (setsockopt(this->socketfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        g_warning("Error configuring socket.");
        this->closeSocket();
        return false;
    }

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = INADDR_ANY;
    if (bind(this->socketfd, reinterpret_cast<sockaddr *>(&server), sizeof(server)) == -1) {
        this->closeSocket();
        g_warning("Error binding socket.");
        return false;
    }

    g_debug("Socket bound");

    if (listen(this->socketfd, 4) == -1) {
        this->closeSocket();
        g_warning("Error listening.");
        return false;
    }

    return true;
}

bool SocketHandler::connectTo(const Address &serverAddress) {
    sockaddr_in server = {};
    this->socketfd = socket(AF_INET, SOCK_STREAM, 0);

    if (!this->isOpen()) {
        g_warning("Error opening socket");
        return false;
    }

    g_debug("Socket opened");

    server.sin_addr.s_addr = inet_addr(serverAddress.getIp().c_str());
    server.sin_family = AF_INET;
    server.sin_port = htons(serverAddress.getPort());

    if (connect(this->socketfd, reinterpret_cast<struct sockaddr *>(&server), sizeof(server)) == -1) {
        this->closeSocket();
        g_warning("Error connecting to server");
        return false;
    }

    g_debug("Connected to server");

    return true;
}

void SocketHandler::closeSocket() {
    if (this->socketfd != CLOSED) {
        close(this->socketfd);
        this->socketfd = CLOSED;
    }
}

bool SocketHandler::isOpen() const {
    return this->socketfd != CLOSED;
}

SocketHandler SocketHandler::acceptConnections() const {
    if (this->socketfd == CLOSED) {
        return {};
    }

    SocketHandler client(accept(this->socketfd, nullptr, nullptr));

    if (!client.isOpen()) {
        g_warning("Error accepting connection");
        return {};
    }

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
        g_warning("Error setting timeout");
        return false;
    }

    return true;
}

READ_RETURN_CODE SocketHandler::readFromSocketInternal() {
    if (this->socketfd == CLOSED) {
        return READ_RETURN_CODE::FAILED;
    }


    if (read(this->socketfd, this->data.data(), BUFFER_SIZE + 3) < 1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return READ_RETURN_CODE::TIMEOUT;
        }
        return READ_RETURN_CODE::FAILED;
    }

    // Check if the message is a single control char message
    if (data.front() == static_cast<byte>(CONTROL_CHAR::STC)) {
        return READ_RETURN_CODE::READ_CONTROL_CHAR;
    }

    // The message actually contains data, we check if the data has been corrupted (STX, ETX, LRC check)
    if (data.front() != static_cast<byte>(CONTROL_CHAR::STX) ||
        *(data.end() - 2) != static_cast<byte>(CONTROL_CHAR::ETX)) {
        return READ_RETURN_CODE::FAILED;
    }

    byte lrc{0};
    for (const auto it: span{data.begin(), data.end() - 1}) {
        lrc ^= it;
    }

    if (lrc != data.back()) {
        return READ_RETURN_CODE::FAILED;
    }

    return READ_RETURN_CODE::READ_DATA;
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

    return (write(this->socketfd, buffer.data(), BUFFER_SIZE + 3) < 1);
}

bool SocketHandler::writeToSocket() {
    this->buffer[0] = static_cast<byte>(CONTROL_CHAR::STX);
    return this->writeToSocketInternal();
}

bool SocketHandler::sendControlChar(CONTROL_CHAR ch) {
    this->buffer[0] = static_cast<byte>(CONTROL_CHAR::STC);
    this->buffer[1] = static_cast<byte>(ch);
    return this->writeToSocketInternal();
}

READ_RETURN_CODE SocketHandler::readFromSocket() {
    const READ_RETURN_CODE ret = this->readFromSocketInternal();
    if (ret == READ_RETURN_CODE::FAILED || ret == READ_RETURN_CODE::TIMEOUT) {
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
