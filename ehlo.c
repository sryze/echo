#include <stdio.h>
#include <stdlib.h>
#include "ehlo-shared.h"

static void print_prompt(void)
{
  printf_locked("> ");
  fflush(stdout);
}

static void *command_thread(void *arg)
{
  socket_t sock = *((socket_t *)arg);

  for (;;) {
    int recv_size;
    int8_t cmd;

    recv_size = recv(sock, &cmd, 1, 0);
    if (recv_size <= 0) {
      if (recv_size == 0) {
        printf_locked("Connection closed\n");
        exit(EXIT_SUCCESS);
      } else {
        printf_locked("Failed to read command: %s\n",
                      error_to_str(socket_error(), NULL, 0));
        continue;
      }
    }

    switch (cmd) {
      case EHLO_CMD_PING:
        /* not implemented yet */
        break;
      case EHLO_CMD_MESSAGE: {
        int16_t client_id;
        char message[EHLO_MAX_MESSAGE_LEN];
        size_t offset = 0;
        recv_size =
            recv_n(sock, (char *)&client_id, sizeof(client_id), 0, NULL);
        if (recv_size <= 0) {
          fprintf_locked(stderr,
                         "Failed to receive message: %s\n",
                         error_to_str(socket_error(), NULL, 0));
          break;
        }
        client_id = ntohs(client_id);
        /*
         * Read the message one chracter at a time until we hit the trailing
         * NUL ('\0') character. This is probably inefficient...
         */
        for (;
             (recv_size = recv(sock, message + offset, 1, 0)) >= 0;
             offset++) {
          if (recv_size == 0 || message[offset] == '\0') {
            /* End of message */
            break;
          }
          if (offset >= sizeof(message)) {
            /* Message is too long, skip remaining text */
            continue;
          }
        }
        if (client_id == EHLO_SERVER_ID) {
          printf_locked("\r[server]: %s\n", message);
        } else {
          printf_locked("\r[%d]: %s\n", client_id, message);
        }
        print_prompt();
        break;
      }
      default:
        fprintf_locked(stderr, "Received unknown command %d\n", cmd);
        break;
    }
  }

  return NULL;
}

int main(int argc, char **argv)
{
  int error;
  socket_t sock;
  struct addrinfo ai_hints, *ai_result = NULL, *ai_cur;
  const char *host, *port;
  char *addr_str = NULL;
  thread_t command_thread_handle;

  if (argc < 3) {
    fprintf(stderr, "Usage: %s <host> <port>\n", get_program_name(argv[0]));
    exit(EXIT_FAILURE);
  }

  host = argv[1];
  port = argv[2];

  socket_init();
  atexit(socket_shutdown);

  sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sock == INVALID_SOCKET) {
    fprintf(stderr, "socket: %s\n",
        error_to_str(socket_error(), NULL, 0));
    goto fatal_error;
  }

  memset(&ai_hints, 0, sizeof(ai_hints));
  ai_hints.ai_family = AF_INET;
  ai_hints.ai_socktype = SOCK_STREAM;
  ai_hints.ai_protocol = IPPROTO_TCP;

  error = getaddrinfo(host, port, &ai_hints, &ai_result);
  if (error != 0) {
    fprintf(stderr,
        "Failed to resolve address: %s\n", gai_strerror(error));
    goto fatal_error;
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
    fprintf(stderr,
        "Could not connect: %s\n", error_to_str(socket_error(), NULL, 0));
    goto fatal_error;
  }

  freeaddrinfo(ai_result);
  ai_result = NULL;

  error = create_thread(&command_thread_handle, command_thread, &sock);
  if (error != 0) {
    fprintf(stderr,
            "Failed to create command thread: %s\n",
            error_to_str(error, NULL, 0));
    goto fatal_error;
  }

  for (;;) {
    char line[EHLO_MAX_MESSAGE_LEN];
    int8_t cmd;

    print_prompt();

    if (fgets(line, sizeof(line), stdin) == NULL) {
      if (feof(stdin)) {
        printf("EOF\n");
      } else {
        fprintf(stderr, 
                "Error reading input: %s\n", 
                error_to_str(errno, NULL, 0));
      }
      break;
    }

    /* Remove trailing newline */
    line[strlen(line) - 1] = '\0';

    /* Send this message to the server */
    cmd = EHLO_CMD_MESSAGE;
    send_n(sock, (char *)&cmd, 1, 0);
    send_n(sock, line, (int)strlen(line) + 1, 0);
  }

  cancel_thread(command_thread_handle);
  close_socket(sock);
  free(addr_str);
  exit(EXIT_SUCCESS);

fatal_error:
  freeaddrinfo(ai_result);
  free(addr_str);
  close_socket(sock);
  exit(EXIT_FAILURE);
}
