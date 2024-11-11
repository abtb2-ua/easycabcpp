#include "Socket_Handler.h"
#include <unistd.h>

using namespace std;

SocketHandler::SocketHandler() {
    s = CLOSED;
}

SocketHandler::SocketHandler(const int s) {
    this->openSocket(s);
}

SocketHandler::~SocketHandler() {
    this->closeSocket();
}

void SocketHandler::openSocket(const int s) {
    this->s = s;

    // TODO: Implement socket opening
}

void SocketHandler::closeSocket() {
    if (this->s != CLOSED) {
        close(this->s);
        this->s = CLOSED;
    }
}