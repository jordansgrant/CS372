/*********************************************************************************
 ** Program: 	chatclient
 ** Project: 	CS372 Project 1
 ** Author:  	Jordan Grant (grantjo)
 ** File: 		chatclient.c
 ** Function:	Main Entry point to the chatclient application. Responsible for
 **				setting up client-server status, creating socket connection, setting up
 **       ncurses windows, and handing off chat services to chat_send_t and
 **       chat_recv_t
 *********************************************************************************/
#include "chat_lib.h"
#include <pthread.h>

int main( int argc, char **argv ) {
  int sock_fd,                            // socket file descripter
      port,                               // connection port
      serv = 1,                           // flag for server status (0 = client)
      *close_d = malloc(sizeof(int)),     // flag to signal end of connection
      i;                                  // loop var

  socklen_t cli_addr_size;               // stores size of sock_addr_in for client
  size_t buffer_size,                     // stores # characters returned by getline
         hand_cap = 12;                   // limit of handle including nul term and
                                          // network sigil

  // storage struct to write host information to
  struct hostent *server_host;

  // socket structs for server and client info
  struct sockaddr_in serv_addr,
                     cli_addr;

  char *buffer = NULL,                          // buffer for getline calls
       *handle = calloc(hand_cap, sizeof(char)),// user handle
       *c;                                      // general use char pointer

  // declare 2 threads
  pthread_t threads[2];
  // declare send and receive chat_sock structs (see chat_lib.h)
  chat_sock send,
            recv;
  // if no port was specified present usage message
  if (argc < 2) {
    fprintf(stderr, "usage: %s port [host]\n", argv[0]);
    return 1;
  }
  // make sure that the port argument is made of at least 1 digit
  // print usage message if not
  if (!isdigit(argv[1][0])) {
    fprintf(stderr, "usage %s port [host]", argv[0]);
    return 1;
  }
  // if 2 arguments were specified, attmpt a client connection
  if (argc > 2) {
    serv = 0;
  }

  // prompt and get users handle
  printf("Whats your handle?\n");
  buffer_size = getline(&handle, &hand_cap, stdin);
  if (buffer_size < 0) {
    fprintf(stderr, "Failed to get handle. Error: %s\n", strerror(errno));
    return 1;
  }
  // strip trailing newline
  c = strchr(handle, '\n');
  *c = 0;
  // copy handle into send and recv chat_socks
  strcpy(send.handle, handle);
  strcpy(recv.handle, handle);
  // add sigil to handle for sending over socket
  handle[strlen(handle)] = '\n';

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

      *close_d = 0;                     // clear close flag
      send.close = close_d;             // set close flag for send thread
      send.sock_fd = conn_fd;           // set socket file descripter for send thread

      recv.close = close_d;             // set close flag for recv thread
      recv.sock_fd = conn_fd;           // set socket file descripter for recv thread

      // Get initial mesage from client containing their handle
      buffer_size = getSockMessage(recv.sock_fd, &buffer);
      if (buffer_size < 0) {
        fprintf(stderr, "Failed to read from socket\n");
        continue;
      }

      // initialize ncurses main window
      WINDOW *stdscr = initscr();
      // return user input immediately rather than waiting for newline
      cbreak();
      // don't echo every key press to the screen
      noecho();
      // initialize color for ncurses
      start_color();
      // initialize 3 color schemes
      init_pair(1, COLOR_GREEN, COLOR_BLACK);
      init_pair(2, COLOR_CYAN, COLOR_BLACK);
	    init_pair(3, COLOR_YELLOW, COLOR_BLACK);
      // get size of 1/2 of the console on the y axis
      int lines = (LINES - 1) / 2;
      // initialize send window to the top half
      send.win = subwin(stdscr,lines, COLS-1, 0, 0);
      // initialize recv window to the bottom half
      recv.win = subwin(stdscr,lines, COLS-1, lines+1, 0);
      // getch should return immediatley with ERR if no keys inputs ready
      nodelay(stdscr, TRUE);
      // ncurses will not automatically handle special keys but rather return
      // them as special user input characters defined in ncurses.h
      keypad(send.win, TRUE);
      // ncurses will scroll when the boundary of the window is reached
      scrollok(send.win, TRUE);
      scrollok(recv.win, TRUE);
      // refresh both new windows
      wrefresh(send.win);
      wrefresh(recv.win);
      // Write a message to host that [hostname] has connected
	    wattron(recv.win,COLOR_PAIR(3));                 //turn on yellow print
	    waddstr(recv.win, "You are now connected to ");  // write to recv window
	    waddstr(recv.win, buffer);
	    waddch(recv.win, '\n');
	    wrefresh(recv.win);
	    wattroff(recv.win,COLOR_PAIR(3));                // turn off yellow print
      // free receive buffer
      free(buffer);
      buffer = NULL;    // set to null for next getline call

      // Create receive thread and pass it the recv chat_sock variables
      int success = pthread_create(&threads[0], NULL, &chat_recv_t, &recv);
      if (success != 0) {
        printf("Failed to create thread: %s\n", strerror(success));
        close(sock_fd);
        return 1;
      }
      // Create send thread and pass it the send chat_sock variables
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
      // clear windows for next connection
      wclear(send.win);
      wclear(recv.win);
      wrefresh(send.win);
      wrefresh(recv.win);
      // reset terminal to normal settings
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

    *close_d = 0;                     // clear close flag
    send.close = close_d;             // set close flag for send thread
    send.sock_fd = sock_fd;           // set socket file descripter for send thread

    recv.close = close_d;             // set close flag for recv thread
    recv.sock_fd = sock_fd;           // set socket file descripter for recv thread

    // send handle to server to initiate Connection
    sendSockMessage(send.sock_fd, handle, strlen(handle));

    // initialize ncurses main window
    WINDOW *stdscr = initscr();
    // return user input immediately rather than waiting for newline
    cbreak();
    // don't echo every key press to the screen
    noecho();
    // initialize color for ncurses
    start_color();
    // initialize 2 color schemes
    init_pair(1, COLOR_GREEN, COLOR_BLACK);
    init_pair(2, COLOR_CYAN, COLOR_BLACK);
    // get size of 1/2 of the console on the y axis
    int lines = (LINES - 1) / 2;
    // initialize send window to the top half
    send.win = subwin(stdscr,lines, COLS-1, 0, 0);
    // initialize recv window to the bottom half
    recv.win = subwin(stdscr,lines, COLS-1, lines+1, 0);
    // getch should return immediatley with ERR if no keys inputs ready
    nodelay(stdscr, TRUE);
    // ncurses will not automatically handle special keys but rather return
    // them as special user input characters defined in ncurses.h
    keypad(send.win, TRUE);
    // ncurses will scroll when the boundary of the window is reached
    scrollok(send.win, TRUE);
    scrollok(recv.win, TRUE);
    // refresh both new windows
    wrefresh(send.win);
    wrefresh(recv.win);

    // Create receive thread and pass it the recv chat_sock variables
    int success = pthread_create(&threads[0], NULL, &chat_recv_t, &recv);
    if (success != 0) {
      printf("Failed to create thread: %s\n", strerror(success));
      close(sock_fd);
      return 1;
    }
    // Create send thread and pass it the send chat_sock variables
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
