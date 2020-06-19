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

  sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sock == -1) {
    fprintf(stderr, "socket: %s\n",
        get_error_string(get_socket_error(), NULL, 0));
    return 3;
  }

  memset(&ai_hints, 0, sizeof(ai_hints));
  ai_hints.ai_family = AF_INET;
  ai_hints.ai_protocol = IPPROTO_TCP;
  ai_hints.ai_socktype = SOCK_STREAM;

  error = getaddrinfo(argv[1], argv[2], &ai_hints, &ai_result);
  if (error != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(error));
    close(sock);
    return 4;
  }

  for (ai_cur = ai_result; ai_cur != NULL; ai_cur = ai_cur->ai_next) {
    error = connect(sock,
                    (struct sockaddr *)ai_cur->ai_addr,
                    (int)ai_cur->ai_addrlen);
    if (error == 0) {
      break;
    }
  }

  if (ai_cur == NULL) {
    fprintf(stderr, "connect: %s\n",
        get_error_string(get_socket_error(), NULL, 0));
    freeaddrinfo(ai_result);
    close(sock);
    return 5;
  }

  puts("I connected!");

  close(sock);

  #ifdef _WIN32
    WSACleanup();
  #endif
}
