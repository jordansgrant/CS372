#include <arpa/inet.h>
#include "fileserve_lib.h"

int conn_count = 0,
    close_d = 0;

char *comm_list = "cd <path>&ls&get <file>&help&exit\n"; 

// Signal Handler for SIGCHLD
// Simply decrements open connection count then reaps child process
void handle_SIGCHLD(int signo) {
  conn_count--;
  int exit_val;
  wait(&exit_val);
}

// Signal Handler for SIGINT
// Sets the close flag and reaps children
void handle_SIGINT(int signo) {
  pid_t wpid;
  int status;
  while ( (wpid = wait(&status)) > 0 ) {}
  close_d = 1;
}

int main( int argc, char **argv ) {
  // port needs to be specified at launch
  if (argc < 2 || !isdigit((int)argv[1][0])) {
    fprintf(stderr, "Usage: %s <port>\n", argv[0]);
    return 1;
  }

  struct sigaction SIGCHLD_action = {{0}},
                   SIGINT_action = {{0}};

  // set up SIGCHLD handler to toggle foreground-only mode
  SIGCHLD_action.sa_handler = handle_SIGCHLD;
  // fill sa_mask so subsequent signals will block until the current returns
  sigfillset(&SIGCHLD_action.sa_mask);
  // set the SA_RESTART flag so that sys calls will attempt re-entry rather
  // than failing with an error when SIGCHLD is handled
  SIGCHLD_action.sa_flags = SA_RESTART;

  // set up SIGINT to handle exiting cleanly
  SIGINT_action.sa_handler = handle_SIGINT;
  // fill sa_mask so subsequent signals will block until the current returns
  sigfillset(&SIGINT_action.sa_mask);
  SIGINT_action.sa_flags = 0;


  // register SIGCHLD hanlder with kernel
  sigaction(SIGCHLD, &SIGCHLD_action, NULL);
  // register SIGINT hanlder with kernel
  sigaction(SIGINT, &SIGINT_action, NULL);


  int success,
      listen_fd, conn_fd,
      port_num,
      buff_size,
      retval,
      exit_d;
  // used in accept calls
  socklen_t cli_addr_size;
  // used for forking child switch case
  pid_t spawn_pid,
        myid;

  char *buffer = NULL,
       cli_address[20],
       *c,
       *conn_exceed = "Error: Connections Exceeded\n";

  // socket structs for server and client info
  struct sockaddr_in serv_addr,
                     cli_addr;

  // clear server struct
  memset((char *)&serv_addr, '\0', sizeof(serv_addr));
  port_num = atoi(argv[1]);                   // get port from argv
  serv_addr.sin_family = AF_INET;             // speficy full socket functionality
  serv_addr.sin_port = htons(port_num);       // convert port to network byte order
  serv_addr.sin_addr.s_addr = INADDR_ANY;     // connect on any address

  // Open a new socket
  listen_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_fd < 0){
    perror("fileserve Error Opening Socket");
    return 1;
  }
  // bind the spefified port and display error if failed
  if (bind(listen_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    perror("fileserve Error binding port");
    return 1;
  }
  // listen for incoming connections
  listen(listen_fd, CONN_COUNT);
  // need size of client address struct for accept call
  cli_addr_size = sizeof(cli_addr);
  // Loop on accept to handle mutiple requests
  do {
    // accept incoming connection and assign its fild descriper to fd
    // also write client information to cli_addr
    conn_fd = accept(listen_fd, (struct sockaddr *)&cli_addr, &cli_addr_size);
    if (conn_fd < 0) {
      if (close_d)
        break;
      fprintf(stderr, "Connect Error: %s\n", strerror(errno));
      continue;
    }
    // If Connection count has been exceeded tell the client, close socket,
    // and continue
    if (conn_count  >= CONN_COUNT) {
      buff_size = getSockMessageCmd(conn_fd, &buffer);
      sendSockMessage(conn_fd, conn_exceed, strlen(conn_exceed));
      close(conn_fd);
      free(buffer);
      buffer = NULL;
      continue;
    }
    // get the address of the connected client
    inet_ntop(AF_INET, &(cli_addr.sin_addr), cli_address, INET_ADDRSTRLEN);
    
    // If we are under the max connections, fork off a child to handle the
    // interaction
    spawn_pid = fork();
    switch(spawn_pid) {
      case -1:
        //Error spawning child, close socket and print error
        fprintf(stderr, "fileserve Fork Error! Child Process creation failed\n");
        close(conn_fd);
        break;
        // Child starts here
      case 0:
        // get pid of child
        myid = getpid();
        printf("Connection initiated with child %d\n", myid);
        exit_d = 0;       // client sends exit command
        do {
          // get message from client, check for errors
          retval = getSockMessageCmd(conn_fd, &buffer);
          if (retval < 0) {
            fprintf(stderr, "Child %d failed to read socket and closed\n", myid);
            close(conn_fd);
            exit(2);
          }
          // split command by & if present
          c = strchr(buffer, '&');
          if ( c != NULL ) {
            *c = 0;
            c++;
          }
          // handle change directory, check for errors
          if ( strstr(buffer, "cd") != NULL ) {
            retval = handle_cd(buffer, conn_fd);
            exit_d = (retval == -2) ? 1 : 0;
          }
          // handle exit request, alert client we got the message
          else if ( strstr(buffer, "exit") != NULL ) {
            retval = sendSockMessage(conn_fd, "Exiting\n", 8);
            exit_d = 1;
          }
          // handle list request check for errors
          else if ( strstr(buffer, "ls") != NULL ) { 
            retval = handle_ls(buffer, c, cli_address, conn_fd);
            exit_d = (retval == -2) ? 1 : 0;
          }
          // handle the get request, check for errors
          else if ( strstr(buffer, "get") != NULL ) {
            retval = handle_get(buffer, c,cli_address, conn_fd);
            exit_d = (retval == -2) ? 1 : 0;
          }
          else if ( strstr(buffer, "help" ) != NULL ){
            // send command list to client
            retval = sendSockMessage(conn_fd, comm_list, strlen(comm_list));
            if ( retval < 0 ) {
              fprintf(stderr, "Child %d failed to write socket and closed\n", myid);
              exit_d = 1;
            }
          }
          else {
            // command not known alert client
            retval = sendSockMessage(conn_fd, "Error: unknown command\n", 23);
            if (retval < 0) {
              fprintf(stderr, "Child %d failed to write socket and closed\n", myid);
              exit_d = 1;
            }
          }
          
          free(buffer);
          buffer = NULL;
        } while ( !exit_d );
        // close connection to child when exiting
        close(conn_fd);
        printf("Child %d has exited\n", myid);
        break;
        // Close socket and increment connections in parent
      default:
        conn_count++;
        close(conn_fd);
        break;
    }
  // loop until close_d set
  } while( !close_d  );
  // close listen fd
  close(listen_fd);
}
