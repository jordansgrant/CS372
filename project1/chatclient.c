/*********************************************************************************
 ** Program: 	chatclient
 ** Project: 	CS372 Project 1
 ** Author:  	Jordan Grant (grantjo)
 ** File: 		chatclient.c
 ** Function:	Main Entry point to the chatclient application. Responsible for
 **				setting up client-server status, creating socket connection, and
 **				handing off chat services to chat_send_t and chat_recv_t
 *********************************************************************************/
#include "chat_lib.h"
#include <pthread.h>


int main( int argc, char **argv ) {
  int sock_fd,
      port,
      serv = 1,
      *close_d = malloc(sizeof(int)),
      cli_addr_size,
      i;
  size_t buffer_cap = 0,
         buffer_size,
         hand_cap = 12;

  // storage struct to write host information to
  struct hostent *server_host;

  // socket structs for server and client info
  struct sockaddr_in serv_addr,
                     cli_addr;

  char *buffer = NULL,
       *handle = calloc(hand_cap, sizeof(char)),     // hostname
       *c;                                      // general use char pointer

  pthread_t threads[2];

  chat_sock send,
            recv;

  if (argc < 2) {
    fprintf(stderr, "usage: %s port [host]\n", argv[0]);
    return 1;
  }
  if (argc > 2) {
    serv = 0;
  }
  else if (!isdigit(argv[1][0])) {
    fprintf(stderr, "usage %s port [host]", argv[0]);
    return 1;
  }

  printf("Whats your handle?\n");
  buffer_size = getline(&handle, &hand_cap, stdin);
  if (buffer_size < 0) {
    fprintf(stderr, "Failed to get handle. Error: %s\n", strerror(errno));
    return 1;
  }
  c = strchr(handle, '\n');
  *c = 0;

  strcpy(send.handle, handle);
  strcpy(recv.handle, handle);
  handle[strlen(handle)] = '#';
  send.serv = serv;
  recv.serv = serv;

  // clear serv_addr with null chars
  memset((char*)&serv_addr, '\0', sizeof(serv_addr));
  port = atoi(argv[1]);             // convert port to integer
  serv_addr.sin_family = AF_INET;   // Set socket to full functionality
  serv_addr.sin_port = htons(port); // change port to network byte ordering

  if (serv) {
    serv_addr.sin_addr.s_addr = INADDR_ANY;     // connect on any address

    // Open a new socket
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0){
      perror("Error Opening Socket");
      return 1;
    }
    // bind the spefified port and display error if failed
    if (bind(sock_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
      perror("Error binding port");
      return 1;
    }
    // listen for incoming connections
    listen(sock_fd, CONN_COUNT);
    // need size of client address struct for accept call
    cli_addr_size = sizeof(cli_addr);
    while (1) {
      printf("Waiting for a new connection with a chatclient\n");
      // accept incoming connection and assign its fild descriper to fd
      // also write client information to cli_addr
      int conn_fd = accept(sock_fd, (struct sockaddr *)&cli_addr, &cli_addr_size);
      if (conn_fd < 0) {
        fprintf(stderr, "Connect Error: %s\n", strerror(errno));
        continue;
      }
      
      *close_d = 0;
      send.close = close_d;
      send.sock_fd = conn_fd;

      recv.close = close_d;
      recv.sock_fd = conn_fd;

      buffer_size = getSockMessage(recv.sock_fd, &buffer);
      if (buffer_size < 0) {
        fprintf(stderr, "Failed to read from socket\n");
        continue;
      }
      

      WINDOW *stdscr = initscr();
      cbreak();
      noecho();
      start_color();
      init_pair(1, COLOR_GREEN, COLOR_BLACK);
      init_pair(2, COLOR_CYAN, COLOR_BLACK);
	  init_pair(3, COLOR_RED, COLOR_BLACK);

      int lines = (LINES - 1) / 2;
      send.win = subwin(stdscr,lines, COLS-1, 0, 0);
      recv.win = subwin(stdscr,lines, COLS-1, lines+1, 0);
      nodelay(stdscr, TRUE);
      
      keypad(send.win, TRUE);
      scrollok(send.win, TRUE);

      scrollok(recv.win, TRUE);
      
      wrefresh(send.win);
      wrefresh(recv.win);
	  
	  attron(COLOR_PAIR(3));
	  waddstr(recv.sock, "You are now connected to ");
	  waddstr(recv.sock, buffer);
	  waddch(recv.sock, '\n');
	  wrefresh(recv.sock);
	  attroff(COLOR_PAIR(3));

      free(buffer);
      buffer = NULL;

      int success = pthread_create(&threads[0], NULL, &chat_recv_t, &recv);
      if (success != 0) {
        printf("Failed to create thread: %s\n", strerror(success));
        close(sock_fd);
        return 1;
      }
      success = pthread_create(&threads[1], NULL, &chat_send_t, &send);
      if (success != 0) {
        printf("Failed to create thread: %s\n", strerror(success));
        close(sock_fd);
        return 1;
      }

      // call join on each thread to wait for them to end
      for (i = 0; i < 2; i++)
        if (pthread_join(threads[i], NULL) != 0)
          perror("Failed Thread Join\n");
      
      wclear(send.win);
      wclear(recv.win);
      wrefresh(send.win);
      wrefresh(recv.win);
      endwin();
    }
  }
  else {
    // get host information of localhost and print error/exit on failure
    server_host = gethostbyname(argv[2]);
    if (server_host == NULL) {
      fprintf(stderr, "Error: Could not resolve %s\n", argv[2]);
      return 1;
    }
    // write hostname into serv_addr to create socket
    memcpy((char*)&serv_addr.sin_addr.s_addr,
        (char*)server_host->h_addr,
        server_host->h_length);

    // Make a call to socket to get a filedescripter
    // On error, print a message and exit
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
      fprintf(stderr, "Error: %s\n", strerror(errno));
      return 1;
    }
    // connect to server using socket file descripter
    // print error and exit 2 on failure to connect
    if (connect(sock_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
      close(sock_fd);
      fprintf(stderr, "Error: could not connect host: %s port: %d\n", argv[2], port);
      return 2;
    }

    *close_d = 0;
    recv.close = close_d;
    recv.sock_fd = sock_fd;

    send.close = close_d;
    send.sock_fd = sock_fd;

    sendSockMessage(send.sock_fd, handle, strlen(handle));

    WINDOW *stdscr = initscr();
    cbreak();
    noecho();
    start_color();
    init_pair(1, COLOR_GREEN, COLOR_BLACK);
    init_pair(2, COLOR_CYAN, COLOR_BLACK);

    int lines = (LINES - 1) / 2;
    send.win = subwin(stdscr,lines, COLS-1, 0, 0);
    recv.win = subwin(stdscr,lines, COLS-1, lines+1, 0);

    nodelay(stdscr, TRUE);

    keypad(send.win, TRUE);
    scrollok(send.win, TRUE);

    scrollok(recv.win, TRUE);
    
    wrefresh(send.win);
    wrefresh(recv.win);

    int success = pthread_create(&threads[0], NULL, &chat_recv_t, &recv);
    if (success != 0) {
      printf("Failed to create thread: %s\n", strerror(success));
      close(sock_fd);
      return 1;
    }
    success = pthread_create(&threads[1], NULL, &chat_send_t, &send);
    if (success != 0) {
      printf("Failed to create thread: %s\n", strerror(success));
      close(sock_fd);
      return 1;
    }

    // call join on each thread to wait for them to end
    for (i = 0; i < 2; i++)
      if (pthread_join(threads[i], NULL) != 0)
        perror("Failed Thread Join\n");

  }

  delwin(send.win);
  delwin(recv.win);
  delwin(stdscr);
  endwin();
  return 0;
}
