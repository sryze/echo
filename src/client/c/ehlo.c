#include <errno.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #define close closesocket
#else
  #include <netdb.h>
  #include <unistd.h>
  #include <sys/types.h>
  #include <sys/socket.h>
#endif

#ifdef _WIN32
  typedef SOCKET socket_t;
#else
  typedef int socket_t;
#endif

#ifdef _WIN32

static char *get_error_string(int error, char *buf, size_t size)
{
  static char static_buf[1024];
  char *real_buf;
  size_t real_size;

  if (buf != NULL) {
    real_buf = buf;
    real_size = size;
  } else {
    real_buf = static_buf;
    real_size = sizeof(static_buf);
  }

  FormatMessageA(
    FORMAT_MESSAGE_FROM_SYSTEM,
    NULL,
    error,
    0,
    real_buf,
    (DWORD)real_size,
    NULL);

  return real_buf;
}

static int get_socket_error(void)
{
  return WSAGetLastError();
}

#else /* _WIN32 */

static char *get_error_string(int error, char *buf, size_t size)
{
  if (buf != NULL) {
    strerror_r(error, buf, size);
    return buf;
  } else {
    return strerror(error);
  }
}

static int get_socket_error(void)
{
  return errno;
}

#endif /* !_WIN32 */

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

  memset(&ai_hints, sizeof(ai_hints), 0);
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
