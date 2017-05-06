/*******************************************************************************
 ** Program:
 ** Project: CS344 Program 4 OTP
 ** Author: Jordan Grant (grantjo)
 *******************************************************************************/
// include otp library header file
#include "chat_lib.h"

/*******************************************************************************
 ** Function: chat_recv_t
 ** Paramaters: void* containing
 ** Description: receive thread entry point. Receives and displays messages from
 **				remote client/server
 ** Return: void*
 *******************************************************************************/
void* chat_recv_t(void* args) {
  chat_sock sock = *(chat_sock *)args;
  char *c;

  do {
    char *buffer = NULL;
    int buffer_size;

    buffer_size = getSockMessage(sock.sock_fd, &buffer);
    if (buffer_size < 0) {
      *(sock.close) = 1;
      close(sock.sock_fd);
      return NULL;
    }
    else if (strstr(buffer, "\\quit") != NULL) {
      waddstr(sock.win, "\rConnection was closed by peer\n");
      wrefresh(sock.win);

      sleep(1);
      free(buffer);

      *(sock.close) = 1;
      close(sock.sock_fd);
      return NULL;
    }
    c = strchr(buffer, '>');
    *c = 0;
    c++;

    prompt(sock, buffer, 2);
    waddstr(sock.win, c);
    waddch(sock.win, '\n');
    wrefresh(sock.win);

    free(buffer);

  } while ( !*(sock.close) );
  return NULL;
}

/*******************************************************************************
 ** Function: chat_send_t
 ** Paramaters: void* containing
 ** Description: receive thred entry point. Receives and displays messages from
 **				remote client/server
 ** Return: void*
 *******************************************************************************/
void* chat_send_t(void *args){
  chat_sock sock = *(chat_sock *)args;
  int retval;
  size_t buff_cap = BUFFER_START_SIZE,
         buff_size = 0;
  char *buffer = calloc(buff_cap, sizeof(char));
  prompt(sock, sock.handle, 1);
  do {
    int ch;

    if ((ch = getch()) != ERR) {
      if (buff_size + 1 >= buff_cap) {
        buff_cap *= 2;
        resizeBuffer(&buffer, buff_cap);
      }
      switch (ch) {
        case KEY_DC:
        case KEY_BACKSPACE:
        case ALT_BACKSPACE:
          buff_size--;
          buffer[buff_size] = 0;
          int y, x;
          getyx(sock.win,y, x);
          mvwdelch(sock.win, y, x-1);
          wrefresh(sock.win);
          break;
        case KEY_ENTER:
        case ALT_ENTER:
          if (strstr(buffer, "\\quit") != NULL) {
            buffer[buff_size++] = '#';
            sendSockMessage(sock.sock_fd, buffer, strlen(buffer));
            *(sock.close) = 1;
            close(sock.sock_fd);
            free(buffer);
            return NULL;
          }
          if (buff_size > 0) {
            buffer[buff_size++] = '#';
            prepend_handle(sock.handle, &buff_cap, &buffer);
            retval = sendSockMessage(sock.sock_fd, buffer, strlen(buffer));
            if (retval < 0) {
              waddstr(sock.win, "Failed to write to socket :(\n");
              wrefresh(sock.win);
              close(sock.sock_fd);
              *(sock.close) = 1;
              return NULL;
            }
            buff_size = 0;
            memset(buffer, 0, buff_cap);
          }
          waddch(sock.win,'\n');
          prompt(sock, sock.handle, 1);
          wrefresh(sock.win);
          break;
        default:
          if (isprint(ch)) {
            buffer[buff_size] = ch;
            buff_size += 1;
            waddch(sock.win, ch);
            wrefresh(sock.win);
          }
          break;
      }
    }
  } while ( !*(sock.close) );
  return NULL;
}



/*******************************************************************************
 ** Function: getSockMessage
 ** Paramaters: int file descripter of socket connection
 ** Description: reads entire message terminated with '#' sigil from socket.
 ** Return: -1 if error, number of bytes read if successful
 *******************************************************************************/
int getSockMessage(int fd, char **buffer) {
  // if buffer is NULL allocate with default size and initialize all to null
  if (*buffer == NULL)
    *buffer = calloc(BUFFER_START_SIZE, sizeof(char));
  // set initial buffer capacity and size
  int buff_cap = BUFFER_START_SIZE,
      buff_size = 0;
  // char pointer for searching buffer
  char *c;
  // loop until full message is read
  do {  // while ( (c = strchr(*buffer, '#')) == NULL);
    // allocate buffer for recv
    char *temp = calloc(BUFFER_START_SIZE + 1, sizeof(char));
    int chars_recv;
    // read message from buffer
    chars_recv = recv(fd, temp, BUFFER_START_SIZE, 0);
    // if recv error print message and return -1
    if (chars_recv < 0) {
      return -1;
    }
    // if buffer doesnt have enough capacity resize and update buff_cap
    if (buff_size + chars_recv >= buff_cap) {
      buff_cap *= 3;
      resizeBuffer(buffer, buff_cap);
    }
    // add bytes read to buffer and update buff_size
    strcat(*buffer, temp);
    buff_size += chars_recv;
    // free temp buffer
    free(temp);
    // loop until message termination sigil is found
  } while ( (c = strchr(*buffer, '#')) == NULL);
  // replace sigil with null terminator
  *c = 0;
  // return size of buffer
  return buff_size;
}

/*******************************************************************************
 ** Function: sendSockMessage
 ** Paramaters: int file descripter of socket connection
 **             char** buffer to send
 **             int size of buffer to send
 ** Description: sends size characters into socket.
 ** Return: -1 if error, number of bytes sent if successful
 *******************************************************************************/
int sendSockMessage(int fd, char *buff, int size) {
  int sent = 0,         // # bytes sent
      to_send = size,   // # bytes yet to send
      retval = 0;       // return value from send()
  // loop until size bytes are sent
  while (sent < size) {
    // send remaining bytes to socket
    retval = send(fd, buff+sent, to_send, 0);
    // if there is a send error print message and return
    if ( retval < 0 ) {
      return -1;
    }
    // update number of bytes sent and number of bytes left to send
    sent += retval;
    to_send -= retval;
  }
  // return sent
  return sent;
}

/*******************************************************************************
 ** Function: resizeBuffer
 ** Paramaters: char** buffer to resize
 **             int new capacity size
 ** Description: reallocates buffer to size new_cap.
 **              buffer contents remain and empty bytes are set to null char
 ** Return: resized buffer in char**, void
 *******************************************************************************/
void resizeBuffer(char **buffer, int new_cap) {
  // allocate a buffer of size new_cap full of null chars
  char *temp = calloc(new_cap,sizeof(char));
  // copy buffer into temp
  strcpy(temp, *buffer);
  // free old buffer
  free(*buffer);
  // assign new buffer to old pointer
  *buffer = temp;
}

void prepend_handle(char* handle, size_t *buff_cap, char** buffer) {
  if (strlen(handle) + strlen(*buffer) > *buff_cap) {
    *buff_cap *= 2;
    resizeBuffer(buffer, *buff_cap);
  }
  char *temp = calloc(*buff_cap, sizeof(char));
  sprintf(temp, "%s>%s#", handle, *buffer);
  strcpy(*buffer, temp);
  free(temp);
}

void prompt(chat_sock sock, char *prompt, int color) {
  int len = strlen(prompt),
      i = 0;

  for (; i < len; i++)
    waddch(sock.win, prompt[i] | COLOR_PAIR(color));
  waddch(sock.win, '>' | COLOR_PAIR(color));
  waddch(sock.win, ' ');
  wrefresh(sock.win);
}
