#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #include <windows.h>
  #define close_socket closesocket
  #define strdup _strdup
  #define SHUT_RD SD_RECEIVE
  #define SHUT_WR SD_SEND
  #define SHUT_RDWR SD_BOTH
#else
  #include <netdb.h>
  #include <pthread.h>
  #include <unistd.h>
  #include <arpa/inet.h>
  #include <netinet/in.h>
  #include <sys/types.h>
  #include <sys/socket.h>
  #define close_socket close
#endif
#ifndef INVALID_SOCKET
  #define INVALID_SOCKET -1
#endif
#ifndef INVALID_THREAD
  #define INVALID_THREAD 0
#endif

#ifdef _WIN32
  typedef int socklen_t;
  typedef SOCKET socket_t;
  typedef HANDLE thread_t;
  typedef HANDLE mutex_t;
#else
  typedef int socket_t;
  typedef pthread_t thread_t;
  typedef pthread_mutex_t mutex_t;
#endif

#ifdef _MSC_VER
  typedef signed __int8 int8_t;
  typedef unsigned __int8 uint8_t;
  typedef signed __int16 int16_t;
  typedef unsigned __int16 uint16_t;
  typedef signed __int32 int32_t;
  typedef unsigned __int32 uint32_t;
  typedef signed __int64 int64_t;
  typedef unsigned __int64 uint64_t;
#else
  #include <stdint.h>
#endif

typedef int (*recv_handler_t)(
    const char *buf, int len, int chunk_offset, int chunk_len);

enum {
  EHLO_CMD_HELLO = 1,
  EHLO_CMD_MESSAGE = 2,
  EHLO_CMD_PING = 3,
  EHLO_CMD_PONG = 4
};

#define EHLO_MAX_MESSAGE_LEN 128
#define EHLO_MAX_CLIENTS 32

#define EHLO_SERVER_ID -1

const char *get_program_name(const char *path);

void socket_init(void);
void socket_cleanup(void);
int close_socket_nicely(socket_t sock);

int socket_error(void);
char *error_to_str(int error, char *buf, size_t size);

int create_thread(thread_t *thread, void *(*start)(void *arg), void *arg);
int cancel_thread(thread_t thread);
int create_mutex(mutex_t *mutex);
int lock_mutex(mutex_t *mutex);
int unlock_mutex(mutex_t *mutex);
int destroy_mutex(mutex_t *mutex);

int vfprintf_locked(FILE *file, const char *format, va_list args);
int fprintf_locked(FILE *file, const char *format, ...);
int printf_locked(const char *format, ...);

#ifdef _WIN32
  int vasprintf(char **strp, const char *format, va_list args);
  int asprintf(char **strp, const char *format, ...);
#endif

int recv_n(
    socket_t sock, char *buf, int size, int flags, recv_handler_t handler);
int send_n(socket_t sock, const char *buf, int size, int flags);
