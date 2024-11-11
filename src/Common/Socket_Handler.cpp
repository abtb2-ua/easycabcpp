#include "Socket_Handler.h"
#include <sys/socket.h>
#include <cstdio>
#include <unistd.h>
#include <glib.h>
#include <netinet/in.h>

using namespace std;


SocketHandler::SocketHandler() {
    this->s = CLOSED;
}

SocketHandler::~SocketHandler() {
    this->closeSocket();
}

bool SocketHandler::openSocket(const int port) {
    constexpr int opt = 1;
    this->s = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in server = {};

    if (this->s == -1) {
        g_warning("Error opening socket.");
        return false;
    }

    if (setsockopt(this->s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Error en setsockopt");
        close(this->s);
        exit(EXIT_FAILURE);
    }

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = INADDR_ANY;
    if (bind(this->s, reinterpret_cast<sockaddr *>(&server), sizeof(server)) == -1) {
        close(this->s);
        this->s = -1;
        g_warning("Error binding socket.");
        return false;
    }

    g_debug("Socket bound");

    if (listen(this->s, 4) == -1) {
        close(this->s);
        this->s = CLOSED;
        g_warning("Error listening.");
        return false;
    }

    return true;
}

bool SocketHandler::connectTo(const int port) {

    return true;
}

void SocketHandler::closeSocket() {
    if (this->s != CLOSED) {
        close(this->s);
        this->s = CLOSED;
    }
}
