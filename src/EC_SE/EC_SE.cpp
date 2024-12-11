#include <Socket/SocketHandler.h>
#include <arpa/inet.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <glib.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include "Common.h"

int main() {
    ProducerWrapper producer;
    // cout << getTimestamp() << endl;
    // for (int i = 0; i < 100000; i++) {
    //     getTimestamp();
    // }
    // return 0;
    Log log(code_logs::MESSAGE::DEBUG, "Hellof, world!");
    producer.produce("logs", log);
    return 0;
}
