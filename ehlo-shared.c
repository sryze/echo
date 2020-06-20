#include <limits.h>
#include <stdlib.h>
#include "ehlo-shared.h"

static int stdio_lock_created;
static mutex_t stdio_lock;

const char *get_program_name(const char *path)
{
  size_t len = strlen(path);
  const char *p;

  if (len == 0) {
    return NULL;
  }
  for (p = path + len - 1; p > path; p--) {
    if (*p == '/' || *p == '\\') {
      return p + 1;
    }
  }
  return path;
}

#ifdef _WIN32

struct thread_info {
  void *(*start)(void *arg);
  void *arg;
};

void socket_init(void)
{
  int wsa_error;
  WSADATA wsa_data;

  wsa_error = WSAStartup(MAKEWORD(2, 2), &wsa_data);
  if (wsa_error != 0) {
    fprintf(stderr, "WSAStartup: %s\n", error_to_str(wsa_error, NULL, 0));
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

static DWORD WINAPI thread_proc(LPVOID param)
{
  struct thread_info thread_info = *(struct thread_info *)param;

  free(param);
  thread_info.start(thread_info.arg);
  return 0;
}

int create_thread(thread_t *thread, void *(*start)(void *arg), void *arg)
{
  struct thread_info *thread_info;
  HANDLE thread_handle;

  thread_info = malloc(sizeof(*thread_info));
  if (thread_info == NULL) {
    return errno;
  }

  thread_info->start = start;
  thread_info->arg = arg;

  thread_handle = CreateThread(NULL, 0, thread_proc, thread_info, 0, NULL);
  if (thread_handle != NULL) {
    *thread = thread_handle;
    return 0;
  }

  return GetLastError();
}

int cancel_thread(thread_t thread)
{
  return TerminateThread(thread, 0) ? 0 : GetLastError();
}

int create_mutex(mutex_t *mutex)
{
  *mutex = CreateMutex(NULL, FALSE, NULL);
  return *mutex == NULL ? GetLastError() : 0;
}

int lock_mutex(mutex_t *mutex)
{
  return WaitForSingleObjectEx(*mutex, INFINITE, FALSE)
      == WAIT_FAILED ? GetLastError() : 0;
}

int unlock_mutex(mutex_t *mutex)
{
  return ReleaseMutex(*mutex) ? 0 : GetLastError();
}

int destroy_mutex(mutex_t *mutex)
{
  return CloseHandle(*mutex) ? 0 : GetLastError();
}

int vasprintf(char **strp, const char *format, va_list args)
{
  int len;
  size_t size;
  char *str;
  int result;

  len = _vscprintf(format, args);
  if (len == -1) {
      return -1;
  }

  size = (size_t)len + 1;
  str = malloc(size);
  if (str == NULL) {
      return -1;
  }

  result = vsprintf_s(str, len + 1, format, args);
  if (result == -1) {
      free(str);
      return -1;
  }

  *strp = str;
  return result;
}

int asprintf(char **strp, const char *format, ...)
{
  va_list args;
  int result;

  va_start(args, format);
  result = vasprintf(strp, format, args);
  va_end(args);

  return result;
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

int create_thread(thread_t *thread, void *(*start)(void *arg), void *arg)
{
  return pthread_create(thread, NULL, start, arg);
}

int cancel_thread(thread_t thread)
{
  return pthread_cancel(thread);
}

int create_mutex(mutex_t *mutex)
{
  return pthread_mutex_init(mutex, NULL);
}

int lock_mutex(mutex_t *mutex)
{
  return pthread_mutex_lock(mutex);
}

int unlock_mutex(mutex_t *mutex)
{
  return pthread_mutex_unlock(mutex);
}

int destroy_mutex(mutex_t *mutex)
{
  return pthread_mutex_destroy(mutex);
}

#endif /* !_WIN32 */

int vfprintf_locked(FILE *file, const char *format, va_list args)
{
  int result;

  if (!stdio_lock_created) {
    if (create_mutex(&stdio_lock) == 0) {
      stdio_lock_created = 1;
    }
  }
  if (!stdio_lock_created) {
    return -1;
  }

  lock_mutex(&stdio_lock);
  result = vfprintf(file, format, args);
  unlock_mutex(&stdio_lock);

  return result;
}

int fprintf_locked(FILE *file, const char *format, ...)
{
  va_list args;
  int result;

  va_start(args, format);
  result = vfprintf_locked(file, format, args);
  va_end(args);

  return result;
}

int printf_locked(const char *format, ...)
{
  va_list args;
  int result;

  va_start(args, format);
  result = vfprintf_locked(stdout, format, args);
  va_end(args);

  return result;
}

int recv_n(
    socket_t sock, char *buf, int size, int flags, recv_handler_t handler)
{
  int len = 0;
  int recv_len;

  for (;;) {
    if (len >= size) {
      break;
    }
    recv_len = recv(sock, buf + len, size - len, flags);
    if (recv_len <= 0) {
      return recv_len;
    }
    if (recv_len == 0) {
      break;
    }
    len += recv_len;
    if (handler != NULL && handler(buf, len, len - recv_len, recv_len)) {
      break;
    }
  }

  return len;
}

int send_n(socket_t sock, const char *buf, int size, int flags)
{
  int len = 0;
  int send_len;

  for (;;) {
    if (len >= size) {
      break;
    }
    send_len = send(sock, buf + len, size - len, flags);
    if (send_len <= 0) {
      return send_len;
    }
    if (send_len == 0) {
      break;
    }
    len += send_len;
  }

  return len;
}
