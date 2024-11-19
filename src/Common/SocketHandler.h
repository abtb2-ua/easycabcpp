#ifndef SOCKET_HANDLER_H
#define SOCKET_HANDLER_H
#include "Common.h"

class SocketHandler {
    static constexpr int CLOSED = -1;
    bool closeAtEnd = true;
    int s;

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

    // Canonical form
    SocketHandler();

    /// @note It's recommended to use the openSocket method instead of this constructor.
    /// However, if you still find this method more convenient, check the setCloseAtEnd method.
    explicit SocketHandler(int s);

    SocketHandler(SocketHandler &other) = delete;

    SocketHandler(SocketHandler &&other) noexcept;

    SocketHandler &operator=(SocketHandler &other) = delete;

    SocketHandler &operator=(SocketHandler &&other) noexcept;

    ~SocketHandler();

    // Methods
    bool openSocket(int port);

    bool connectTo(const Address &serverAddress);

    void closeSocket();

    [[nodiscard]] bool isOpen() const;

    [[nodiscard]] SocketHandler acceptConnections() const;

    bool setTimeout(int seconds, int microseconds = 0) const; // NOLINT(*-use-nodiscard)

    READ_RETURN_CODE readFromSocket(char buffer[BUFFER_SIZE]) const;

    bool writeToSocket(const char buffer[BUFFER_SIZE]) const;

    bool sendCode(CODE code) const; // NOLINT(*-use-nodiscard)

    /// @brief Defines if the socket should be closed when the object is destroyed. Default is true.
    /// @note Generally it's not recommended to change this value.
    void setCloseAtEnd(const bool closeAtEnd) { this->closeAtEnd = closeAtEnd; }
};

#endif //SOCKET_HANDLER_H
