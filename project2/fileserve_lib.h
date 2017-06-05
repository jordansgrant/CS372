/*******************************************************************************
** Program: project 1, chatclient
** Project: CS372 Project 1
** Author: Jordan Grant (grantjo)
*******************************************************************************/
#ifndef FILESERVE_LIB_H
#define FILESERVE_LIB_H

#include <ctype.h>            // contains functions to text chars
#include <dirent.h>
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

#define CONN_COUNT 100          // max connection backlog
#define BUFFER_START_SIZE 100 // start size of resv buffer
#define READ_BLOCK_LIMIT 500  // amount of data to read at once
int handle_cd(char *cmd, int cmd_fd);

int handle_ls(char *cmd, char *port, char *cli_addr, int cmd_fd);

int handle_get(char *cmd, char *port, char *cli_addr, int cmd_fd);

void prepend_size(char **buffer, char *size, size_t *capacity);

/*******************************************************************************
 ** Function: getSockMessageData
 ** Paramaters: int file descripter of socket connection
 ** Description: reads data from socket. Reads buffer size (delimited with space)        **              then reads buff_size characters from socket.
 ** Return: -1 if error, number of bytes read if successful
 *******************************************************************************/
int getSockMessageData(int fd, char **buffer);

/*******************************************************************************
** Function: getSockMessageCmd
** Paramaters: int file descripter of socket connection
**             char** buffer to read into
** Description: reads entire message terminated with '\n' sigil from socket.
** Return: -1 if error, number of bytes read if successful
*******************************************************************************/
int getSockMessageDataCmd(int fd, char **buffer);

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

#endif
