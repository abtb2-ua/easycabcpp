#ifndef SOCKET_HANDLER_H
#define SOCKET_HANDLER_H
#include "Common.h"

class SocketHandler {
private:
    int s;
    static constexpr int CLOSED = -1;

public:
    enum class CODE {
        ENQ = 0x05,
        ACK = 0x06,
        NACK = 0x15,
        EOT = 0x04,
        STX = 0x02,
        ETX = 0x03
    };

    enum class READ_RETURN_CODE {
        SUCCESS,
        FAILED,
        TIMEOUT
    };

    static int constexpr BUFFER_SIZE = 100;

    SocketHandler();

    explicit SocketHandler(int s);

    SocketHandler(SocketHandler &other) = delete;

    ~SocketHandler();

    SocketHandler &operator=(SocketHandler &&other) noexcept;

    bool openSocket(int port);

    bool connectTo(const Address &serverAddress);

    void closeSocket();

    [[nodiscard]] bool isOpen() const;

    [[nodiscard]] SocketHandler acceptConnections() const;

    bool setTimeout(int seconds, int microseconds = 0) const; // NOLINT(*-use-nodiscard)

    READ_RETURN_CODE readFromSocket(char buffer[BUFFER_SIZE]) const;

    bool writeToSocket(const char buffer[BUFFER_SIZE]) const;

    [[nodiscard]] bool sendCode(CODE code) const;
};

#endif //SOCKET_HANDLER_H
