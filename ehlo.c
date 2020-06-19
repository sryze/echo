#include <stdlib.h>
#include "ehlo-shared.h"

int main(int argc, char **argv)
{
  int error;
  socket_t sock;
  struct addrinfo ai_hints, *ai_result, *ai_cur;
  #ifdef _WIN32
    int wsa_error;
    WSADATA wsa_data;
  #endif
  const char *host, *port;
  char *addr_str;

  if (argc < 3) {
    fprintf(stderr, "Usage: %s <host> <port>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  host = argv[1];
  port = argv[2];

  socket_init();
  atexit(socket_shutdown);

  sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sock == -1) {
    fprintf(stderr, "socket: %s\n",
        error_to_str(socket_error(), NULL, 0));
    exit(EXIT_FAILURE);
  }

  memset(&ai_hints, 0, sizeof(ai_hints));
  ai_hints.ai_family = AF_INET;
  ai_hints.ai_socktype = SOCK_STREAM;
  ai_hints.ai_protocol = IPPROTO_TCP;

  error = getaddrinfo(host, port, &ai_hints, &ai_result);
  if (error != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(error));
    close_socket(sock);
    exit(EXIT_FAILURE);
  }

  printf("Connecting to %s:%s\n", host, port);

  for (ai_cur = ai_result; ai_cur != NULL; ai_cur = ai_cur->ai_next) {
    error = connect(sock,
                    (struct sockaddr *)ai_cur->ai_addr,
                    (int)ai_cur->ai_addrlen);
    if (error == 0) {
      addr_str = strdup(
          inet_ntoa(((struct sockaddr_in *)ai_cur->ai_addr)->sin_addr));
      break;
    }
  }

  if (ai_cur == NULL) {
    fprintf(stderr, "connect: %s\n",
        error_to_str(socket_error(), NULL, 0));
    freeaddrinfo(ai_result);
    close_socket(sock);
    exit(EXIT_FAILURE);
  }

  puts("I connected!");

  close_socket(sock);
  free(addr_str);
}
