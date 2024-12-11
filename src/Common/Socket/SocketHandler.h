#ifndef SOCKET_HANDLER_H
#define SOCKET_HANDLER_H

#include "Common.h"
#include "Protocols.h"

using namespace prot;

class SocketHandler {
public:
    static size_t constexpr BUFFER_SIZE = 256;

    enum class READ_ERROR : uint8_t {
        SOCKET_CLOSED,
        READ_FAILED,
        INVALID_FORMAT,
        INVALID_LRC,
        TIMEOUT,
        NONE,
    };


private:
    static constexpr int CLOSED = -1;

    int socketfd;
    bool closeAtEnd = true;
    READ_ERROR error = READ_ERROR::NONE;

    // Stores the data that will be written to the socket
    array<byte, BUFFER_SIZE + 3> buffer;
    // Stores the data read from the socket
    array<byte, BUFFER_SIZE + 3> data;


    bool checkDataFormat();
    [[nodiscard]] bool readFromSocketInternal();
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
    [[nodiscard]] bool readFromSocket();

    bool setTimeout(int seconds, int microseconds = 0) const; // NOLINT(*-use-nodiscard)

    [[nodiscard]] bool hasData() const { return this->data[0] == static_cast<byte>(CONTROL_CHAR::STX); }

    [[nodiscard]] bool hasControlChar() const { return this->data[0] == static_cast<byte>(CONTROL_CHAR::STC); }

    /// @brief Returns the error that occurred in the last read operation. If none, NONE is returned.
    [[nodiscard]] READ_ERROR getError() const { return this->error; }

    /// @brief Returns a span of the read data. If no data was read, an empty span is returned.
    [[nodiscard]] span<const byte> getData() const;

    /// @brief Returns the first byte of the buffer as a CONTROL_CHAR.
    /// @details In case that the first byte of the buffer does not correspond to any CONTROL_CHAR, NONE is returned.
    [[nodiscard]] CONTROL_CHAR getControlChar() const;
};

#endif //SOCKET_HANDLER_H
