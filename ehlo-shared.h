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
  #include <arpa/inet.h>
  #include <netinet/in.h>
  #include <sys/types.h>
  #include <sys/socket.h>
#endif

#ifdef _WIN32
  typedef SOCKET socket_t;
#else
  typedef int socket_t;
#endif

int get_socket_error(void);
char *get_error_string(int error, char *buf, size_t size);
