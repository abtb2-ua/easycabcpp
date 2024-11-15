#include <iostream>
#include <Socket_Handler.h>

int main(int argc, char *argv[]) {
    g_log_set_default_handler(log_handler, nullptr);
    SocketHandler client;

    if (!client.connectTo(Address("192.168.0.17", 2400)))
        return 1;
    g_message("Connected to server");

    char buffer[SocketHandler::BUFFER_SIZE];
    strcpy(buffer, "Hola");
    g_message("Sending message to server '%s'", buffer);
    if (!client.writeToSocket(buffer)) {
        g_warning("Error sending message");
        return 1;
    }
    g_message("Message sent");

    char c;
    std::cin >> c;

    return 0;
}
