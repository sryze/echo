#include <errno.h>
#include <stdio.h>
#include <winsock2.h>
#include <windows.h>
#include <sys/types.h>

#ifdef WIN32
  #define close closesocket
#endif

static char *get_error_string(int error, char *buffer, size_t size) {
  #ifdef WIN32
    static char static_buffer[1024];

    char *real_buffer = static_buffer;
    size_t real_size = sizeof(static_buffer);

    if (buffer != NULL) {
      real_buffer = buffer;
      real_size = size;
    }

    FormatMessageA(
      FORMAT_MESSAGE_FROM_SYSTEM,
      NULL,
      error,
      0,
      real_buffer,
      real_size,
      NULL);
    return real_buffer;
  #else
    if (buffer != NULL) {
      strerror_r(error, char *buffer, size_t size);
      return buffer;
    }
    else {
      return strerror(error);
    }
  #endif
}

static int get_socket_error(void) {
  #ifdef WIN32
    return WSAGetLastError();
  #else
    return errno;
  #endif
}

int main(int argc, char **argv) {
  char *host;
  int port;
  int error;
  int server_sock;
  struct sockaddr_in server_addr;
  #ifdef WIN32
    int wsa_error;
    WSADATA wsa_data;
  #endif

  if (argc < 3) {
    fprintf(stderr, "Usage: %s <host> <port>\n", argv[0]);
    return 1;
  }

  host = argv[1];
  port = atoi(argv[2]);

  #ifdef WIN32
    wsa_error = WSAStartup(MAKEWORD(2, 2), &wsa_data);
    if (wsa_error != 0) {
      fprintf(stderr, "WSAStartup: %s\n", get_error_string(wsa_error, NULL, 0));
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
  server_addr.sin_addr.s_addr = inet_addr(host);
  server_addr.sin_port = htons(port);

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
    int client_sock;
    struct sockaddr_in client_addr;
    size_t client_addr_len = sizeof(client_addr);

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

  #ifdef WIN32
    WSACleanup();
  #endif
}
