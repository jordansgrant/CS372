/*******************************************************************************
 ** Program: project 1, chatclient
 ** Project: CS372 Project 1
 ** Author: Jordan Grant (grantjo)
 *******************************************************************************/
// include otp library header file
#include "fileserve_lib.h"

int handle_cd(char *cmd, int cmd_fd) {
  int success;
  char *c = strchr(cmd, ' '),
       message[50];
  if ( c == NULL ) {
    sprintf(message, "%s", "Error: no path specified\n");
    success = sendSockMessage(cmd_fd, message, strlen(message));
    return (success < 0) ? -2 : 0;
  }
  c++;
  success = chdir(c);
  if (success < 0) {
    sprintf(message, "%s: %s\n","Error: ", strerror(errno));
    success = sendSockMessage(cmd_fd, message, strlen(message));
    return (success < 0) ? -2 : 0; 
  }
  
  sprintf(message,  "Success: moved to %s\n", c);
  success = sendSockMessage(cmd_fd, message, strlen(message));
  return (success < 0) ? -2 : 0;
}

int handle_ls(char *cmd, char *port_s, char *cli_addr, int cmd_fd) {
  int data_fd,
      port,
      success;
  struct sockaddr_in cli_info;
  struct hostent *cli_host;
  char message[50],
       *buffer,
       *temp = calloc(BUFFER_START_SIZE, sizeof(char)),
       data_len[15];
  size_t buff_size = 0,
         buff_cap = BUFFER_START_SIZE;
  DIR *d;
  struct dirent *dir;
  if (!isdigit(port_s[0])) {
    sprintf(message, "%s", "Error: invalid port syntax\n");
    success = sendSockMessage(cmd_fd, message, strlen(message));
    return (success < 0) ? -2 : 0;
  }
  cli_host = gethostbyname(cli_addr);
  if (cli_host == NULL) {
    sprintf(message, "Error resolving client hostname");
    success = sendSockMessage(cmd_fd, message, strlen(message));
    return (success < 0) ? -2 : -1;
  }
  memcpy((char*)&cli_info.sin_addr.s_addr, (char*)cli_host->h_addr, cli_host->h_length);
  port = atoi(port_s);
  cli_info.sin_family = AF_INET;
  cli_info.sin_port = htons(port);

  d = opendir(".");
  if (d != NULL)
  {
     while ((dir = readdir(d)) != NULL)
     {
        char file[100];;
        if (dir->d_type == DT_DIR) {
           sprintf(file, "%s/&", dir->d_name);
        } else {
           sprintf(file, "%s&", dir->d_name);
        }
        if (strlen(file) + buff_size >= buff_cap) {
            buff_cap *= 2;
            resizeBuffer(&temp, buff_cap);
        }
        strcat(temp, file);
        buff_size += strlen(file);
        
     }

     closedir(d);
  } else {
    sprintf(message, "Error: %s\n", strerror(errno));
    success = sendSockMessage(cmd_fd, message, strlen(message));
    free(temp);
    return (success < 0) ? -2 : -1;
  }
  
  temp[strlen(temp)-1] = '\n';

  data_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (data_fd < 0) {
    sprintf(message, "Error: %s\n", strerror(errno));
    free(temp);
    success = sendSockMessage(cmd_fd, message, strlen(message));
    return (success < 0) ? -2 : -1;
  }
    
  if (connect(data_fd, (struct sockaddr *)&cli_info, sizeof(cli_info)) < 0) {
    close(data_fd);
    free(temp);
    sprintf(message,"Error: could not connect on Data Port\n");
    success = sendSockMessage(cmd_fd, message, strlen(message));
    return (success < 0) ? -2 : -1;
  }
  success = sendSockMessage(data_fd, temp, strlen(temp));
  if (success < 0) {
    sprintf(message, "Error: Failed to send on Data Socket\n");
    success = sendSockMessage(cmd_fd, message, strlen(message));
    free(temp);
    close(data_fd);
    return (success < 0) ? -2 : -1;
  }
  free(temp);
  buffer = NULL;
 
  buff_size = getSockMessageCmd(cmd_fd, &buffer);
  close(data_fd);

  if ( buff_size < 0 || strstr(buffer, "fin") == NULL ) {
    success = sendSockMessage(cmd_fd, "ack_err\n", 8);
    free(buffer);
    return (success < 0) ? -2 : -1;
  }
  free(buffer);
  success = sendSockMessage(cmd_fd, "ack_fin\n", 8);
  return (success < 0) ? -2 : 0;
}

int handle_get(char *cmd, char *port_s, char *cli_addr, int cmd_fd) {
  int data_fd,
      port,
      retval;
  struct sockaddr_in cli_info;
  struct hostent *cli_host;
  char message[100],
       *buffer,
       data_len[15], 
       *c;
  size_t buff_size = 0,
         buff_cap = 0;
  unsigned long file_size;
  DIR *d;
  FILE *file = NULL;
  struct dirent *dir;

  if (!isdigit(port_s[0])) {
    sprintf(message, "%s", "Error: invalid port syntax\n");
    retval = sendSockMessage(cmd_fd, message, strlen(message));
    return (retval < 0) ? -2 : 0;
  }
  c = strchr(cmd, ' ');
  *c = 0;
  c++;

  cli_host = gethostbyname(cli_addr);
  memcpy((char*)&cli_info.sin_addr.s_addr, (char*)cli_host->h_addr, cli_host->h_length);
  port = atoi(port_s);
  cli_info.sin_family = AF_INET;
  cli_info.sin_port = htons(port);
  file = fopen(c, "rb");
  if (file == NULL) {
    sprintf(message, "Error: %s\n", strerror(errno));
    retval = sendSockMessage(cmd_fd, message, strlen(message));
    return (retval < 0) ? -2 : 0;
  }
  fseek(file, 0, SEEK_END);
  file_size = ftell( file );
  fseek( file, 0, SEEK_SET);
  sprintf(data_len, "%lu ", file_size);

  data_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (data_fd < 0) {
    sprintf(message, "Error: %s\n", strerror(errno));
    retval = sendSockMessage(cmd_fd, message, strlen(message));
    return (retval < 0) ? -2 : -1;
  }
    
  if (connect(data_fd, (struct sockaddr*)&cli_info, sizeof(cli_info)) < 0) {
    close(data_fd);
    sprintf(message, "Error: could not connect on Data Port\n");
    retval = sendSockMessage(cmd_fd, message, strlen(message));
    return (retval < 0) ? -2 : -1;
  }
  //int size_sent = 0;
  retval = sendSockMessage(data_fd, data_len, strlen(data_len));
  buffer = calloc(READ_BLOCK_LIMIT, sizeof(char));
  while (fgets(buffer, READ_BLOCK_LIMIT, file) != NULL) {
    //if (!size_sent) {
    //  prepend_size(&buffer, data_len, &buff_cap);
    //  size_sent = 1;
   // }
    retval = sendSockMessage(data_fd, buffer, strlen(buffer));
    if ( retval < 0 ) {
      fclose(file);
      close(data_fd);
      free(buffer);
      sprintf(message, "Error: failed to write to Data Connection\n");
      retval = sendSockMessage(cmd_fd, message, strlen(message));
      return (retval < 0) ? -2 : -1;
    }
  }
  fclose(file);
  free(buffer);
  buffer = NULL;

  buff_size = getSockMessageCmd(cmd_fd, &buffer);
  close(data_fd);

  if ( buff_size < 0 || strstr(buffer, "fin") == NULL ) {
    retval = sendSockMessage(cmd_fd, "ack_err\n", 8);
    free(buffer);
    return (retval < 0) ? -2 : -1;
  }
  free(buffer);
  retval = sendSockMessage(cmd_fd, "ack_fin\n", 8);
  return (retval < 0) ? -2 : 0;
}


void prepend_size(char **buffer, char *size, size_t *capacity) {
  int buff_size = strlen(*buffer) + strlen(size) + 1;
  char *temp = calloc(buff_size, sizeof(char));
  strcpy(temp, size);
  strcat(temp, *buffer);
  free(*buffer);
  *buffer = temp;
  *capacity = buff_size;
}

/*******************************************************************************
 ** Function: getSockMessageData
 ** Paramaters: int file descripter of socket connection
 ** Description: reads data from socket. Reads buffer size (delimited with space)        **              then reads buff_size characters from socket.
 ** Return: -1 if error, number of bytes read if successful
 *******************************************************************************/
int getSockMessageData(int fd, char **buffer) {
  // if buffer is NULL allocate with default size and initialize all to null
  if (*buffer == NULL)
    *buffer = calloc(BUFFER_START_SIZE, sizeof(char));
  // set initial buffer capacity and size
  int buff_cap = BUFFER_START_SIZE,
      buff_size = 0,
      buff_len_found = 0,
      buff_len;
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

    if (!buff_len_found) {
      c = strchr(temp, ' ');
      c = 0;
      buff_len = atoi(temp);
      c++;
    } else {
      c = temp;
    }

    // if buffer doesnt have enough capacity resize and update buff_cap
    if (buff_size + chars_recv >= buff_cap) {
      buff_cap *= 3;
      resizeBuffer(buffer, buff_cap);
    }
    // add bytes read to buffer and update buff_size
    strcat(*buffer, c);
    buff_size += chars_recv;

    // free temp buffer
    free(temp);
    // loop until message termination sigil is found
  } while ( !buff_len_found || buff_size < buff_len );
  // replace sigil with null terminator
  *c = 0;
  // return size of buffer
  return buff_size;
}

/*******************************************************************************
 ** Function: getSockMessageCmd
 ** Paramaters: int file descripter of socket connection
 **             char** buffer to read into
 ** Description: reads entire message terminated with '\n' sigil from socket.
 ** Return: -1 if error, number of bytes read if successful
 *******************************************************************************/
int getSockMessageCmd(int fd, char **buffer) {
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

