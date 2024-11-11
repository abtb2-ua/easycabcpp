#ifndef SOCKET_HANDLER_H
#define SOCKET_HANDLER_H

class SocketHandler {
private:
    int s;
    static constexpr int CLOSED = -1;

public:
    SocketHandler();
    explicit SocketHandler(int s);
    ~SocketHandler();

    void openSocket(int s);
    void closeSocket();
};

#endif //SOCKET_HANDLER_H
