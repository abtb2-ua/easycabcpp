#ifndef SOCKET_HANDLER_H
#define SOCKET_HANDLER_H

class SocketHandler {
private:
    static constexpr int CLOSED = -1;
    int s;

public:
    SocketHandler();
    ~SocketHandler();

    bool openSocket(int s);
    bool connectTo(int s);

    void closeSocket();
};

#endif //SOCKET_HANDLER_H
