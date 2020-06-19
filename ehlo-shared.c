#include "ehlo-shared.h"

#ifdef _WIN32

int get_socket_error(void)
{
  return WSAGetLastError();
}

char *get_error_string(int error, char *buf, size_t size)
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

int get_socket_error(void)
{
  return errno;
}

char *get_error_string(int error, char *buf, size_t size)
{
  if (buf != NULL) {
    strerror_r(error, buf, size);
    return buf;
  } else {
    return strerror(error);
  }
}

#endif /* !_WIN32 */
