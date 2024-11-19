#include <arpa/inet.h>
#include <netinet/in.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/socket.h>
#include <unistd.h>
#include "Common.h"
#include <glib.h>

void cleanUpSocket(int *s) {
  close(*s);
  *s = -1;
}

int openSocket(int port) {
  struct sockaddr_in server;
  int s = socket(AF_INET, SOCK_STREAM, 0);
  int opt = 1;

  if (s == -1) {
    g_error("Error opening socket.");
  }

  if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    perror("Error en setsockopt");
    close(s);
    exit(EXIT_FAILURE);
  }

  server.sin_family = AF_INET;
  server.sin_port = htons(port);
  server.sin_addr.s_addr = INADDR_ANY;
  if (bind(s, (struct sockaddr *)&server, sizeof(server)) == -1) {
    close(s);
    s = -1;
    g_error("Error binding socket.");
  }

  g_message("Socket bound");

  if (listen(s, 4) == -1) {
    close(s);
    s = -1;
    g_error("Error listening.");
  }

  return s;
}

int main(int argc, char *argv[]) {

  int s = openSocket(2400);

  int client = accept(s, NULL, NULL);
  if (client == -1) {
    g_error("Error accepting connection");
  }

  g_message("s: %d", s);
  g_message("client: %d", client);

  while (1) {
    g_message("Connection accepted");

    char buffer[1024];

    if (read(client, buffer, 1024) < 1) {
      g_error("Error reading from client");
    }

    printf("Client sent: %s\n", buffer);
  }
}