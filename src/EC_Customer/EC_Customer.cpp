#include <Socket/SocketHandler.h>


#include "Common.h"
#include "Logs.h"

using namespace std;
using namespace code_logs;

int main(int argc, char *argv[]) {
  SocketHandler fd;
    fd.openSocket(8234);
    auto client = fd.acceptConnections();
    if (client.readFromSocket())
        cout << "Received: " << static_cast<int>(client.getControlChar()) << endl;
    else
        cout << "Error reading from socket" << endl;
}