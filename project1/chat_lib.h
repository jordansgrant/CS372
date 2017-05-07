/*******************************************************************************
** Program: project 1, chatclient
** Project: CS372 Project 1
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

#define CONN_COUNT 0          // max connection backlog
#define BUFFER_START_SIZE 100 // start size of resv buffer

#define ALT_BACKSPACE 127
#define ALT_ENTER 10

// Contains information to be passed to threads for seding/receiving data
struct chat_sock {
	int sock_fd;					// file descripter
	int *close;						// flag to close connection
	char handle[11];			// user handle
	WINDOW *win;					// ncurses window
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
** Description: reads entire message terminated with '\n' sigil from socket.
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

/*******************************************************************************
 ** Function: prepend_handle
 ** Paramaters: char* handle
 **             size_t* buffer capacity
 **             char** buffer to prepend handle to
 ** Description: prepends user handle with prompt deliminator seperating message
 ** Return: void
 *******************************************************************************/
void prepend_handle(char* handle, size_t *buff_cap, char** buffer);

/*******************************************************************************
 ** Function: prompt
 ** Paramaters: chat_sock
 **             char* prompt
 **             int index of color pair
 ** Description: prints prompt to screen with color
 ** Return: void
 *******************************************************************************/
void prompt(chat_sock sock, char *prompt, int color);

#endif
