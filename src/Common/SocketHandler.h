#ifndef SOCKET_HANDLER_H
#define SOCKET_HANDLER_H
#include "Common.h"


enum class CONTROL_CHAR {
    ENQ = 0x05,
    ACK = 0x06,
    NACK = 0x15,
    EOT = 0x04,
    STX = 0x02,
    ETX = 0x03,
    NONE = 0x00,

    // Similar ot STX, the message doesn't contain data but a single byte (a control char). LRC and ETX aren't used.
    STC = 0x01,
};

enum class READ_RETURN_CODE {
    READ_DATA,
    READ_CONTROL_CHAR,
    FAILED,
    TIMEOUT
};

class SocketHandler {
public:
    static size_t constexpr BUFFER_SIZE = 100;

private:
    static constexpr int CLOSED = -1;

    bool closeAtEnd;
    int socketfd;

    // Stores the data that will be written to the socket
    array<byte, BUFFER_SIZE + 3> buffer;
    // Stores the data read from the socket
    array<byte, BUFFER_SIZE + 3> data;


    [[nodiscard]] READ_RETURN_CODE readFromSocketInternal();
    [[nodiscard]] bool writeToSocketInternal();
public:
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

    // Connection handling methods
    bool openSocket(int port);

    bool connectTo(const Address &serverAddress);

    void closeSocket();

    [[nodiscard]] bool isOpen() const;

    [[nodiscard]] SocketHandler acceptConnections() const;

    /// @brief Defines if the socket should be closed when the object is destroyed. Default is true.
    /// @note Generally it's not recommended to change this value.
    void setCloseAtEnd(const bool closeAtEnd) { this->closeAtEnd = closeAtEnd; }


    // Write-related methods
    bool writeToSocket();

    bool sendControlChar(CONTROL_CHAR ch);

    /// @brief Returns a span of the modifiable part of the buffer.
    span<byte> getBuffer() { return {this->buffer.begin() + 1, BUFFER_SIZE}; }


    // Read-related methods
    [[nodiscard]] READ_RETURN_CODE readFromSocket();

    bool setTimeout(int seconds, int microseconds = 0) const; // NOLINT(*-use-nodiscard)

    [[nodiscard]] bool hasData() const { return this->data[0] == static_cast<byte>(CONTROL_CHAR::STX); }

    [[nodiscard]] bool hasControlChar() const { return this->data[0] == static_cast<byte>(CONTROL_CHAR::STC); }

    [[nodiscard]] bool lastReadFailed() const { return this->data[0] == static_cast<byte>(CONTROL_CHAR::NONE); }

    /// @brief Returns a span of the read data. If no data was read, an empty span is returned.
    [[nodiscard]] span<const byte> getData() const;

    /// @brief Returns the first byte of the buffer as a CONTROL_CHAR.
    /// @details In case that the first byte of the buffer does not correspond to any CONTROL_CHAR, NONE is returned.
    [[nodiscard]] CONTROL_CHAR getControlChar() const;
};

#endif //SOCKET_HANDLER_H
