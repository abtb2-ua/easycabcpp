#include <arpa/inet.h>
#include <netinet/in.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/socket.h>
#include <unistd.h>
#include <Common.h>
#include <glib.h>

void cleanUpSocket(int *s) {
  close(*s);
  *s = -1;
}

int connectToServer(Address *serverAddress) {
  sockaddr_in server;
  int s = socket(AF_INET, SOCK_STREAM, 0);

  if (s == -1) {
    g_error("Error opening socket");
  }

  g_message("Socket opened");

  server.sin_addr.s_addr = inet_addr(serverAddress->getIp().c_str());
  server.sin_family = AF_INET;
  server.sin_port = htons(serverAddress->getPort());

  if (connect(s, (struct sockaddr *)&server, sizeof(server)) == -1) {
    close(s);
    s = -1;
    g_error("Error connecting to server");
  }

  g_message("Connected to server");

  return s;
}

int main() {
  Address serverAddress;
  serverAddress.setIp("192.168.0.17");
  serverAddress.setPort(2400);

  const int s = connectToServer(&serverAddress);
  cout << s << endl;

  char buffer[100];
  strcpy(buffer, "Hola");
  buffer[strlen(buffer)] = '\0';

  if (write(s, buffer, 1024) == -1) {
    g_error("Error writing to server");
  }

  close(s);
  return EXIT_SUCCESS;
}