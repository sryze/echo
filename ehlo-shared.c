#include "ehlo-shared.h"

#ifdef _WIN32

void socket_init(void)
{
  int wsa_error;
  WSADATA wsa_data;

  wsa_error = WSAStartup(MAKEWORD(2, 2), &wsa_data);
  if (wsa_error != 0) {
    fprintf(stderr, "WSAStartup: %s\n",
        error_to_str(wsa_error, NULL, 0));
    exit(EXIT_FAILURE);
  }
}

void socket_shutdown(void)
{
  WSACleanup();
}

int socket_error(void)
{
  return WSAGetLastError();
}

char *error_to_str(int error, char *buf, size_t size)
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

#else /* _WIN32 */

void socket_init(void)
{
  /* nothing to do */
}

void socket_shutdown(void)
{
  /* nothing to do */
}

int socket_error(void)
{
  return errno;
}

char *error_to_str(int error, char *buf, size_t size)
{
  if (buf != NULL) {
    strerror_r(error, buf, size);
    return buf;
  } else {
    return strerror(error);
  }
}

#endif /* !_WIN32 */
