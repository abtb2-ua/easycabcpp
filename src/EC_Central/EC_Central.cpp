#include <glib-2.0/glib.h>
#include <iostream>
#include <fstream>
#include "Common.h"
#include <cppkafka/cppkafka.h>
#include <dotenv.h>
#include <Socket_Handler.h>

using namespace std;

class Something {
public:
    int a;
    int b;
};

int main() {
    g_log_set_default_handler(log_handler, nullptr);
    dotenv::init(dotenv::Preserve);
    // if (const string env = dotenv::getenv("EC_CENTRAL_PATH", ""); !env.empty()) {
    //     dotenv::init((env +"/.env").c_str());
    // }

    SocketHandler server;

    server.openSocket(2400);
    g_message("Socket opened on port 10.000. Waiting for connection...");
    const SocketHandler client = server.acceptConnections();
        // g_message("Connection accepted");
        //
        // char buffer[1024];
        //
        // if (read(client.s, buffer, 1024) < 1) {
        //     g_error("Error reading from client");
        // }
        //
        // printf("Client sent: %s\n", buffer);
    if (!client.isOpen()) {
        g_warning("Error accepting connection");
        return 1;
    }
    g_message("Connection accepted");

    char buffer[SocketHandler::BUFFER_SIZE];
    if (client.readFromSocket(buffer) != SocketHandler::READ_RETURN_CODE::SUCCESS) {
        g_warning("Error reading from socket");
        return 1;
    }
    g_message("Message received from client: '%s'", buffer);

    char c;
    cin >> c;

    // cppkafka::Producer producer(getConfig(false));
    //
    //
    //
    // const string key = "some_key";
    // Something smth{};
    // smth.a = 1;
    // smth.b = 2;
    // const cppkafka::Buffer smthBuffer(reinterpret_cast<const char*>(&smth), sizeof(Something));
    //
    // // Build and produce the message
    // producer.produce(cppkafka::MessageBuilder("requests").key(key).payload(smthBuffer));
    // producer.flush();

    return 0;
}