#include <stdlib.h>
#include "ehlo-shared.h"

int main(int argc, char **argv)
{
  int error;
  socket_t server_sock;
  struct sockaddr_in server_addr;
  #ifdef _WIN32
    int wsa_error;
    WSADATA wsa_data;
  #endif

  if (argc < 3) {
    fprintf(stderr, "Usage: %s <host> <port>\n", argv[0]);
    return 1;
  }

  #ifdef _WIN32
    wsa_error = WSAStartup(MAKEWORD(2, 2), &wsa_data);
    if (wsa_error != 0) {
      fprintf(stderr, "WSAStartup: %s\n",
          get_error_string(wsa_error, NULL, 0));
      return 2;
    }
  #endif

  puts("Hello, World!");

  server_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (server_sock == -1) {
    fprintf(stderr, "socket: %s\n",
        get_error_string(get_socket_error(), NULL, 0));
    return 3;
  }

  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = inet_addr(argv[1]);
  server_addr.sin_port = htons(atoi(argv[2]));

  error = bind(server_sock,
               (struct sockaddr *)&server_addr,
               sizeof(server_addr));
  if (error != 0) {
    fprintf(stderr, "bind: %s\n",
        get_error_string(get_socket_error(), NULL, 0));
    close(server_sock);
    return 4;
  }

  error = listen(server_sock, 0);
  if (error != 0) {
    fprintf(stderr, "listen: %s\n",
        get_error_string(get_socket_error(), NULL, 0));
    close(server_sock);
    return 5;
  }

  puts("I'm listening!");

  for (;;) {
    socket_t client_sock;
    struct sockaddr_in client_addr;
    int client_addr_len = sizeof(client_addr);

    client_sock = accept(server_sock,
                         (struct sockaddr *)&client_addr,
                         &client_addr_len);
    if (client_sock == -1) {
      fprintf(stderr, "accept: %s\n",
          get_error_string(get_socket_error(), NULL, 0));
      break;
    }

    printf("Client connected: %s\n", inet_ntoa(client_addr.sin_addr));
    close(client_sock);
  }
  
  close(server_sock);

  #ifdef _WIN32
    WSACleanup();
  #endif
}
