#include <stdio.h>
#include <stdlib.h>
#include "ehlo-shared.h"

static struct client {
  int id;
  socket_t sock;
  thread_t thread;
} clients[EHLO_MAX_CLIENTS];

static int send_message(socket_t sock, int sender_id, const char *message)
{
  int8_t cmd = EHLO_CMD_MESSAGE;
  int16_t client_id = htons(sender_id);
  int result;

  result = send_n(sock, (char *)&cmd, 1, 0);
  if (result <= 0) {
    return socket_error();
  }
  result = send_n(sock, (char *)&client_id, sizeof(client_id), 0);
  if (result <= 0) {
    return socket_error();
  }
  result = send_n(sock, message, (int)(strlen(message) + 1), 0);
  if (result <= 0) {
    return socket_error();
  }
  return 0;
}

static int send_server_message(socket_t sock, const char *message)
{
  return send_message(sock, EHLO_SERVER_ID, message);
}

static void send_broadcast_message(int sender_id, const char *message)
{
  int i;
  int error;

  if (sender_id == EHLO_SERVER_ID) {
    printf_locked("[server]: %s\n", message);
  } else {
    printf_locked("[%d]: %s\n", sender_id, message);
  }

  for (i = 0; i < EHLO_MAX_CLIENTS; i++) {
    if (i != sender_id && clients[i].sock != INVALID_SOCKET) {
      error = send_message(clients[i].sock, sender_id, message);
      if (error != 0) {
        fprintf_locked(stderr,
            "Error sending message to client %d: %s\n",
            i,
            error_to_str(error, NULL, 0));
      }
    }
  }
}

static void send_connect_message(int client_id)
{
  char *buf;

  asprintf(&buf, "Client %d has joined the chat", client_id);
  send_broadcast_message(EHLO_SERVER_ID, buf);
  free(buf);
}

static void send_disconnect_message(int client_id)
{
  char *buf;

  asprintf(&buf, "Client %d has left the chat", client_id);
  send_broadcast_message(EHLO_SERVER_ID, buf);
  free(buf);
}

static void *client_thread(void *arg)
{
  struct client *client = arg;

  send_server_message(client->sock, "Welcome to the chat!");

  for (;;) {
    int recv_size;
    int8_t cmd;

    recv_size = recv(client->sock, (char *)&cmd, 1, 0);
    if (recv_size <= 0) {
      if (recv_size == 0) {
        printf_locked("Client %d disconnected\n", client->id);
        close_socket_nicely(client->sock);
        client->sock = INVALID_SOCKET;
        client->thread = INVALID_THREAD;
        send_disconnect_message(client->id);
      } else {
        printf_locked("Failed to read command from client %d: %s\n",
                      client->id,
                      error_to_str(socket_error(), NULL, 0));
      }
      break;
    }

    switch (cmd) {
      case EHLO_CMD_PING:
        /* not implemented yet */
        break;
      case EHLO_CMD_MESSAGE: {
        char message[EHLO_MAX_MESSAGE_LEN];
        size_t offset = 0;
        /*
         * Read the message one chracter at a time until we hit the trailing
         * NUL ('\0') character. This is probably inefficient...
         */
        for (;
             (recv_size = recv(client->sock, message + offset, 1, 0)) >= 0;
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
        send_broadcast_message(client->id, message);
        break;
      }
      default:
        fprintf_locked(stderr,
           "Received unknown command %d from client %d\n", cmd, client->id);
        break;
    }
  }

  return NULL;
}

int main(int argc, char **argv)
{
  int error;
  socket_t server_sock;
  int opt_reuseaddr;
  struct sockaddr_in server_addr;
  const char *host, *port;
  int i;

  if (argc < 3) {
    fprintf(stderr, "Usage: %s <host> <port>\n", get_program_name(argv[0]));
    exit(EXIT_FAILURE);
  }

  host = argv[1];
  port = argv[2];

  socket_init();
  atexit(socket_cleanup);

  server_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (server_sock == -1) {
    fprintf(stderr, "Failed to open socket: %s\n",
        error_to_str(socket_error(), NULL, 0));
    exit(EXIT_FAILURE);
  }

  /*
   * Allow reuse of this socket address (port) - this makes restarts faster.
   *
   * Sockets are usually waited on for some time by the system after they are
   * closed (netstat shows them in a TIME_WAIT state).
   */
  opt_reuseaddr = 1;
  setsockopt(server_sock,
             SOL_SOCKET,
             SO_REUSEADDR,
             (const void *)&opt_reuseaddr,
             sizeof(opt_reuseaddr));

  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(atoi(port));

  error = bind(server_sock,
               (struct sockaddr *)&server_addr,
               sizeof(server_addr));
  if (error != 0) {
    fprintf(stderr, "Failed to bind address: %s\n",
        error_to_str(socket_error(), NULL, 0));
    close_socket(server_sock);
    exit(EXIT_FAILURE);
  }

  error = listen(server_sock, 128);
  if (error != 0) {
    fprintf(stderr, "Listen error: %s\n",
        error_to_str(socket_error(), NULL, 0));
    close_socket(server_sock);
    exit(EXIT_FAILURE);
  }

  printf("Listening at %s:%s\n", host, port);

  for (i = 0; i < EHLO_MAX_CLIENTS; i++) {
    clients[i].id = i;
    clients[i].sock = INVALID_SOCKET;
    clients[i].thread = INVALID_THREAD;
  }

  for (;;) {
    socket_t client_sock;
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    client_sock = accept(server_sock,
                         (struct sockaddr *)&client_addr,
                         &client_addr_len);
    if (client_sock == INVALID_SOCKET) {
      fprintf_locked(stderr,
                     "Failed to accept connection: %s\n",
                     error_to_str(socket_error(), NULL, 0));
      break;
    }

    for (i = 0; i < EHLO_MAX_CLIENTS; i++) {
      if (clients[i].sock == INVALID_SOCKET) {
        clients[i].sock = client_sock;
        break;
      }
    }

    if (i == EHLO_MAX_CLIENTS) {
      fprintf_locked(stderr,
          "Aborting connection from %s because reached maximum number of clients\n",
          inet_ntoa(client_addr.sin_addr));
      close_socket_nicely(client_sock);
      continue;
    }

    printf_locked(
        "Client connected: %s (%d)\n", inet_ntoa(client_addr.sin_addr), i);

    error = create_thread(&clients[i].thread, client_thread, &clients[i]);
    if (error != 0) {
      fprintf_locked(stderr,
                     "Failed to create client thread: %s\n",
                     error_to_str(error, NULL, 0));
      close_socket_nicely(clients[i].sock);
      clients[i].sock = INVALID_SOCKET;
      clients[i].thread = INVALID_THREAD;
    }

    send_connect_message(i);
  }

  printf("Server is shutting down\n");

  for (i = 0; i < EHLO_MAX_CLIENTS; i++) {
    if (clients[i].sock != INVALID_SOCKET) {
      close_socket_nicely(clients[i].sock);
    }
  }

  close_socket_nicely(server_sock);
}
