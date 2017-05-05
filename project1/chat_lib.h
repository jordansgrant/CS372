/*******************************************************************************
** Program: otp_lib.h (library header)
** Project: CS344 Program 4 OTP
** Author: Jordan Grant (grantjo)
*******************************************************************************/
#ifndef CHAT_LIB_H
#define CHAT_LIB_H

#include <ctype.h>            // contains functions to text chars
#include <errno.h>            // defines errno
#include <ncurses.h>
#include <netdb.h>
#include <netinet/in.h>       // network structs and constants
#include <stdio.h>            // I/O library functions
#include <stdlib.h>           // General typedef's, Constants, and lib functions
#include <string.h>           // library functions to manipulate strings
#include <sys/socket.h>       // socket types and functions
#include <sys/wait.h>         // contains wait functions
#include <unistd.h>           // contains std unix sys calls, like fork

#define CONN_COUNT 5          // max connection count
#define BUFFER_START_SIZE 100 // start size of resv buffer

#define ALT_BACKSPACE 127
#define ALT_ENTER 10

struct chat_sock {
	int sock_fd;
	int *close;
	int serv;
	char handle[11];
	WINDOW *win;
};

typedef struct chat_sock chat_sock;

/*******************************************************************************
** Function: chat_recv_t
** Paramaters: void* containing
** Description: receive thread entry point. Receives and displays messages from
**				remote client/server
** Return: void*
*******************************************************************************/
void* chat_recv_t(void* args);

/*******************************************************************************
** Function: chat_send_t
** Paramaters: void* containing
** Description: receive thred entry point. Receives and displays messages from
**				remote client/server
** Return: void*
*******************************************************************************/
void* chat_send_t(void *args);

/*******************************************************************************
** Function: getSockMessage
** Paramaters: int file descripter of socket connection
**             char** buffer to read into
** Description: reads entire message terminated with '#' sigil from socket.
** Return: -1 if error, number of bytes read if successful
*******************************************************************************/
int getSockMessage(int fd, char **buffer);

/*******************************************************************************
** Function: sendSockMessage
** Paramaters: int file descripter of socket connection
**             char** buffer to send
**             int size of buffer to send
** Description: sends size characters into socket.
** Return: -1 if error, number of bytes sent if successful
*******************************************************************************/
int sendSockMessage(int fd, char *to_send, int size);

/*******************************************************************************
** Function: resizeBuffer
** Paramaters: char** buffer to resize
**             int new capacity size
** Description: reallocates buffer to size new_cap.
**              buffer contents remain and empty bytes are set to null char
** Return: resized buffer in char**, void
*******************************************************************************/
void resizeBuffer(char **buffer, int new_cap);

void prepend_handle(char* handle, size_t *buff_cap, char** buffer);

void prompt(chat_sock sock, char *prompt, int color);

#endif
