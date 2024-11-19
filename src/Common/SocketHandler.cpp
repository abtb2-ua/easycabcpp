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
SocketHandler::SocketHandler(): s(CLOSED) {
}

SocketHandler::SocketHandler(const int s): s(s) {
}

SocketHandler::SocketHandler(SocketHandler &&other) noexcept: s(other.s) {
    other.s = CLOSED;
}

SocketHandler &SocketHandler::operator=(SocketHandler &&other) noexcept {
    if (this == &other) { return *this; }

    if (this->s != CLOSED) {
        close(this->s);
    }

    this->s = other.s;
    other.s = CLOSED;

    return *this;
}

SocketHandler::~SocketHandler() {
    this->closeSocket();
}

bool SocketHandler::openSocket(const int port) {
    constexpr int opt = 1;
    this->s = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in server = {};

    if (!this->isOpen()) {
        g_warning("Error opening socket.");
        return false;
    }

    if (setsockopt(this->s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        g_warning("Error configuring socket.");
        this->closeSocket();
        return false;
    }

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = INADDR_ANY;
    if (bind(this->s, reinterpret_cast<sockaddr *>(&server), sizeof(server)) == -1) {
        this->closeSocket();
        g_warning("Error binding socket.");
        return false;
    }

    g_debug("Socket bound");

    if (listen(this->s, 4) == -1) {
        this->closeSocket();
        g_warning("Error listening.");
        return false;
    }

    return true;
}

bool SocketHandler::connectTo(const Address &serverAddress) {
    sockaddr_in server = {};
    this->s = socket(AF_INET, SOCK_STREAM, 0);

    if (!this->isOpen()) {
        g_warning("Error opening socket");
        return false;
    }

    g_debug("Socket opened");

    server.sin_addr.s_addr = inet_addr(serverAddress.getIp().c_str());
    server.sin_family = AF_INET;
    server.sin_port = htons(serverAddress.getPort());

    if (connect(this->s, reinterpret_cast<struct sockaddr *>(&server), sizeof(server)) == -1) {
        this->closeSocket();
        g_warning("Error connecting to server");
        return false;
    }

    g_debug("Connected to server");

    return true;
}

void SocketHandler::closeSocket() {
    if (this->s != CLOSED && this->closeAtEnd) {
        close(this->s);
        this->s = CLOSED;
    }
}

bool SocketHandler::isOpen() const {
    return this->s != CLOSED;
}

SocketHandler SocketHandler::acceptConnections() const {
    if (this->s == CLOSED) {
        return {};
    }

    SocketHandler client(accept(this->s, nullptr, nullptr));

    if (!client.isOpen()) {
        g_warning("Error accepting connection");
        return {};
    }

    return client;
}

bool SocketHandler::setTimeout(const int seconds, const int microseconds) const {
    if (this->s == CLOSED) {
        return false;
    }

    timeval timeout{};
    timeout.tv_sec = seconds;
    timeout.tv_usec = microseconds;

    if (setsockopt(this->s, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        g_warning("Error setting timeout");
        return false;
    }

    return true;
}

SocketHandler::READ_RETURN_CODE SocketHandler::readFromSocket(char buffer[BUFFER_SIZE]) const {
    if (this->s == CLOSED) {
        g_debug("Attempted to read from closed socket");
        return READ_RETURN_CODE::FAILED;
    }


    char _buffer[BUFFER_SIZE + 3];
    if (read(this->s, _buffer, BUFFER_SIZE + 3) < 1) {
        g_debug("Error reading from socket: %s", strerror(errno));
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return READ_RETURN_CODE::TIMEOUT;
        }

        return READ_RETURN_CODE::FAILED;
    }

    if (_buffer[0] != static_cast<char>(CODE::STX) || _buffer[BUFFER_SIZE + 1] != static_cast<char>(CODE::ETX)) {
        return READ_RETURN_CODE::FAILED;
    }

    unsigned char lrc = 0;
    for (int i = 0; i < BUFFER_SIZE + 2; i++) {
        lrc ^= _buffer[i];
    }
    if (lrc != static_cast<unsigned char>(_buffer[BUFFER_SIZE + 2])) {
        g_debug("Lrc doesn't match");
        return READ_RETURN_CODE::FAILED;
    }

    memcpy(buffer, _buffer + 1, BUFFER_SIZE);

    return READ_RETURN_CODE::SUCCESS;
}

bool SocketHandler::writeToSocket(const char buffer[BUFFER_SIZE]) const {
    if (this->s == CLOSED) {
        return false;
    }

    char _buffer[BUFFER_SIZE + 3];
    unsigned char lrc = 0;

    _buffer[0] = static_cast<char>(CODE::STX);
    lrc ^= _buffer[0];
    for (int i = 0; i < BUFFER_SIZE; i++) {
        _buffer[i + 1] = buffer[i];
        lrc ^= _buffer[i + 1];
    }
    _buffer[BUFFER_SIZE + 1] = static_cast<char>(CODE::ETX);
    lrc ^= _buffer[BUFFER_SIZE + 1];
    _buffer[BUFFER_SIZE + 2] = static_cast<char>(lrc);

    if (write(this->s, _buffer, BUFFER_SIZE + 3) < 1) {
        return false;
    }

    return true;
}

bool SocketHandler::sendCode(const CODE code) const {
    char buffer[BUFFER_SIZE] = {0};
    buffer[0] = static_cast<char>(code);
    return this->writeToSocket(buffer);
}
