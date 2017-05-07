/*******************************************************************************
 ** Program: project 1, chatclient
 ** Project: CS372 Project 1
 ** Author: Jordan Grant (grantjo)
 *******************************************************************************/
// include otp library header file
#include "chat_lib.h"

/*******************************************************************************
 ** Function: chat_recv_t
 ** Paramaters: void* containing struct chat_sock
 ** Description: receive thread entry point. Receives and displays messages from
 **				remote client/server
 ** Return: void*
 *******************************************************************************/
void* chat_recv_t(void* args) {
  // cast chat_sock information
  chat_sock sock = *(chat_sock *)args;
  char *c;

  do {  // while ( !*(sock.close) )
    // declare buffer variables
    char *buffer = NULL;
    int buffer_size;
    // get message from remote peer and close connection if error with socket
    buffer_size = getSockMessage(sock.sock_fd, &buffer);
    if (buffer_size < 0) {
      *(sock.close) = 1;      // set close flag so both threads close
      close(sock.sock_fd);
      return NULL;
    }
    // if a quit message is received close the connection
    else if (strstr(buffer, "\\quit") != NULL) {
      free(buffer);
      *(sock.close) = 1;     // set close flag so both threads close
      close(sock.sock_fd);
      return NULL;
    }
    // split buffer at prompt char
    c = strchr(buffer, '>');
    *c = 0;
    c++;
    // give a color prompt for users handle
    prompt(sock, buffer, 2);
    // print message in standard color
    waddstr(sock.win, c);
    waddch(sock.win, '\n');
    wrefresh(sock.win);

    free(buffer);
    // loop until sock.close is set
  } while ( !*(sock.close) );
  return NULL;
}

/*******************************************************************************
 ** Function: chat_send_t
 ** Paramaters: void* containing
 ** Description: send thread entry point. Gets user input and sends messages to
 **              remote peer.
 ** Return: void*
 *******************************************************************************/
void* chat_send_t(void *args){
  // cast chat_sock from void
  chat_sock sock = *(chat_sock *)args;
  int retval;
  // initialize buffer size information
  size_t buff_cap = BUFFER_START_SIZE,
         buff_size = 0;
  // initialize start buffer
  char *buffer = calloc(buff_cap, sizeof(char));
  // give a color prompt for sender
  prompt(sock, sock.handle, 1);

  do {// while ( !*(sock.close) );
    int ch;
    // get character from keyboard, return immediatley if no input available
    if ((ch = getch()) != ERR) {
      // resize buffer if not sufficient capacity
      if (buff_size + 1 >= buff_cap) {
        buff_cap *= 2;
        resizeBuffer(&buffer, buff_cap);
      }
      // determine proper key effect
      switch (ch) {
        // if a delete was specified remove the last character from the buffer
        case KEY_DC:
        case KEY_BACKSPACE:
        case ALT_BACKSPACE:
          // remove char from buffer if buffer isnt empty
          if (buff_size > 0){
            // decrement buff_size and set last char to null
            buff_size--;
            buffer[buff_size] = 0;
            // get x, y coordinates of cursor
            int y, x;
            getyx(sock.win,y, x);
            // remove last character
            mvwdelch(sock.win, y, x-1);
            wrefresh(sock.win);
          }
          break;
        // If ENTER is pressed submit message or reprompt if no information
        case KEY_ENTER:
        case ALT_ENTER:
          // if message is quit
          if (strstr(buffer, "\\quit") != NULL) {
            // add sigil to message
            buffer[buff_size++] = '\n';
            // send signal to end session to remote peer
            sendSockMessage(sock.sock_fd, buffer, strlen(buffer));
            *(sock.close) = 1;      // set close flag
            close(sock.sock_fd);    // close socket
            free(buffer);
            return NULL;
          }
          // if there is a message to send
          if (buff_size > 0) {
            // add sigil to message
            buffer[buff_size++] = '\n';
            // prepend users handle to message
            prepend_handle(sock.handle, &buff_cap, &buffer);
            // send message. Break conneciton on error
            retval = sendSockMessage(sock.sock_fd, buffer, strlen(buffer));
            if (retval < 0) {
              waddstr(sock.win, "Failed to write to socket :(\n");
              wrefresh(sock.win);
              close(sock.sock_fd);
              *(sock.close) = 1;      // set close flag
              return NULL;
            }
            // reset buffer
            buff_size = 0;
            memset(buffer, 0, buff_cap);
          }
          // reprompt user
          waddch(sock.win,'\n');
          prompt(sock, sock.handle, 1);
          wrefresh(sock.win);
          break;
        default:
          // if a printable character was given add to buffer and print to screen
          if (isprint(ch)) {
            buffer[buff_size] = ch;
            buff_size += 1;
            waddch(sock.win, ch);
            wrefresh(sock.win);
          }
          break;
      }
    }
    // loop while close flag is not set
  } while ( !*(sock.close) );
  return NULL;
}



/*******************************************************************************
 ** Function: getSockMessage
 ** Paramaters: int file descripter of socket connection
 ** Description: reads entire message terminated with '\n' sigil from socket.
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
  do {  // while ( (c = strchr(*buffer, '\n')) == NULL);
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
  } while ( (c = strchr(*buffer, '\n')) == NULL);
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

/*******************************************************************************
 ** Function: prepend_handle
 ** Paramaters: char* handle
 **             size_t* buffer capacity
 **             char** buffer to prepend handle to
 ** Description: prepends user handle with prompt deliminator seperating message
 ** Return: void
 *******************************************************************************/
void prepend_handle(char* handle, size_t *buff_cap, char** buffer) {
  // if handle wont fit in buffer resize
  if (strlen(handle) + strlen(*buffer) + 2 >= *buff_cap) {
    *buff_cap *= 2;
    resizeBuffer(buffer, *buff_cap);
  }
  // allocate temp buffer
  char *temp = calloc(*buff_cap, sizeof(char));
  // write formatted string to new buffer and replace old one
  sprintf(temp, "%s>%s\n", handle, *buffer);
  free(*buffer);
  *buffer = temp;
}
/*******************************************************************************
 ** Function: prompt
 ** Paramaters: chat_sock
 **             char* prompt
 **             int index of color pair
 ** Description: prints prompt to screen with color
 ** Return: void
 *******************************************************************************/
void prompt(chat_sock sock, char *prompt, int color) {
  // set window to color mode
  wattron(sock.win, COLOR_PAIR(color));
  // write prompt to screen and refresn
  waddstr(sock.win, prompt);
  waddch(sock.win, '>' | COLOR_PAIR(color));
  waddch(sock.win, ' ');
  wrefresh(sock.win);
  // turn off color mode
  wattroff(sock.win, COLOR_PAIR(color));
}
