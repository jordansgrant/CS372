#!/usr/bin/python
# Program:  filecli
# Project:  CS372 Project 2
# Author:   Jordan Grant (grantjo)
# File:     filecli.py
# Function: filecli is a python program responsible for makeing a client/server 
#           connection to a remote host and setting up a chat session.
#           
#           The server mode waits on a specified port and accepts new connections
#           until a SIGINT occurs
#           The client mode initiates a connection with a remote server instance
#
#           server mode is specified by only passing a port number to the program
#           client mode is specified by passing a hostname as well as a portnumber
#
#           usage: ./chatserve.py port [host]
#
#           once a connection is made a curses window is partitioned into 2
#           segments, 1 for sent information and 1 for received. Then a separate
#           thread is started for receiving and sending.
#
#           After initial setup the two peers can exchange information until one
#           or the other sends a \quit command terminating the connection 
# EXTRA CREDIT
#
# 1. Program was developed so that either chatclient or chatserve can make the
#    initial communication
# 2. Both hosts may send at anytime without having to trade off 
# 3. Execution of the sending and receiving is done in sepearate threads
# 4. The screen is split using ncurses library so that the user input and 
#    responses are in separate panels
# 5. Color is used to give contrast between the handle prompts and the messages
import os.path
import select
import sys
import socket
import threading

# max connection count for listener
conn_count = 1
# close flag for threads
close = 0

# declare globals for typical backspace and enter characters
ALT_BACKSPACE = 127
ALT_ENTER = 10

# chat_sock class, specifies utilities for creating and managing sockets
# members: sock         - the actual socket connection
#                : recv_max - the maximum size buffer to receive
class file_sock(object):
  # constructer, init with socket passed in or create new AF_INET TCP socket if not
    def __init__(self, sock=None):
      if sock is None:
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
      else:
        self.sock = sock
      # set max receive buffer
      self.recv_max = 2048
    # connect call, calls sockets connect passing tuple of host and port
    def connect(self, host, port):
      self.sock.connect((host, port))
    # serv, creates a server socket ready to start accepting connections
    def serv(self, port):
      # bind for any incoming address on specified port
        self.sock.bind(("", port))
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        # start listening on socket
        self.sock.listen(1)
    def get_serv_port(self):
      return self.sock.getsockname()[1]
    # close, end the socket connection
    def close(self):
        self.sock.shutdown(socket.SHUT_RDWR)
        self.sock.close()
    #accept, accepts a connection to a remote client
    #wraps returned socket in a chat_sock and returns
    def accept(self):
        (client_raw, address) = self.sock.accept()
        client = file_sock(client_raw)
        return client
    # sendSockMessage, sends the entire passed message to the remote peer
    def sendSockMessage(self, msg):
      # get size of message and set sent size to 0
        size = len(msg)
        sent = 0
        # loop until entire message is sent
        while sent < size:
          # send remaining message from sent onward, catch error if socket fails
            retval = self.sock.send(msg[sent:])
            if retval == 0:
              return -1
            # add bytes sent to the total sent so far
            sent += retval
        # return the number of bytes sent
        return sent
    # getSockMessage, receives entire message from socket terminated by sigil '\n'
    def getSockMessage(self):
      # init message variable
        message = ''
        # loop until sigil is found
        while(message.find("\n") < 0):
          # call recv on socket specifying the max size to receive in one call
            temp = self.sock.recv(self.recv_max)
            # if null is returned socket failed
            if temp == '':
              return -1
            # concatenate received onto the full messge
            message = message + temp;
        # return message, less the sigil
        return message.replace("\n", "")
    def getSockStreamFile(self, fname):
      # init message, size, and size_found variables
       size = -1
       size_found = 0
       chars_recv = 0;
       wfile = None;
       head, tail = os.path.split(fname)
       if ( os.path.exists(tail) ):
           file_n, file_e = os.path.splitext(tail)
           i = 1
           while os.path.exists("%s%d%s" % (file_n, i, file_e)):
               i += 1
           tail = "%s%d%s" % (file_n, i, file_e)

       wfile = open(tail, "wb")

       if not wfile:
           return -1

       # loop until message of size is found
       while(size < 0 or chars_recv < size):
         # call recv on socket specifying the max size to recv in one call
           temp = self.sock.recv(self.recv_max)
           #if null is returned socket failed
           if temp == '':
             return -1

           if not size_found:
               split_s = temp.split(" ", 1)
               size = int(split_s[0])
               temp = split_s[1]
               size_found = 1

           chars_recv += len(temp)
           wfile.write(temp)
       return chars_recv

# chat_recv is a class that handles displaying messages received to the user
# it defines a do_thread method meant execute in its own thread
class file_client(object):
  # constructor initializes member variables used during execution
    def __init__(self, sock):
        self.cmd_sock = sock
        self.data_sock = None;
        self.fname= ""
    # save data to file thread
    def save_data_t(self):
        server = self.data_sock.accept()
        server.getSockStreamFile(self.fname)

    def print_data_t(self):
        server = self.data_sock.accept()
        message = server.getSockMessage()
        lines = message.split('&')
        for l in lines:
            print l


# if too few arguments are give display usage message
if len(sys.argv) < 3:
    print >> sys.stderr, "usage: %s port host" % (sys.argv[0])
    sys.exit(1)
# if first argument is not a number give usage message
if not sys.argv[1].isdigit():
    print >> sys.stderr, "usage: %s port host" % (sys.argv[0])
    sys.exit(1)

port = int(sys.argv[1])
# create socket and attempt connection, throw exception on error
sock = file_sock()
sock.connect(sys.argv[2], port)
client = file_client(sock)

exit_d = 0

while not exit_d :
    cmd = raw_input("\nEnter a command or help for list of command\n")

    if cmd == "":
      continue
    elif cmd.find("get") == 0:
      client.data_sock = file_sock()
      client.data_sock.serv(0)
      client.fname = cmd[cmd.find(' ')+1:]
      data_t = threading.Thread(target=client.save_data_t, name="data_t")
      data_t.start()

      client.cmd_sock.sendSockMessage("%s&%d\n" % (cmd,client.data_sock.get_serv_port()))

      error = 0
      while data_t.isAlive():
          r, w, e = select.select([client.cmd_sock.sock], [], [], 0.1)
          if r:
              message = client.cmd_sock.getSockMessage()
              if isinstance( message, (int, long) ):
                  client.data_sock.close()
                  client.data_sock = None
                  print >> sys.stderr, "Error reading from command socket"
              elif message.find("Error") >= 0:
                  print >> sys.stderr, message
                  client.data_sock.close()
                  client.data_sock = None
      data_t.join()
      if not error:
        client.cmd_sock.sendSockMessage("fin\n")
        message = client.cmd_sock.getSockMessage()
        if isinstance( message, (int, long) ):
            print >> sys.stderr, "Error reading from command socket"
            exit_d = 1
            client.cmd_sock.close()

        elif message.find("ack_fin") < 0:
            print >> sys.stderr, "Error in ack from server, %s" % (message)
        client.data_sock.close();
        client.data_sock = None

    elif cmd == "ls":
      client.data_sock = file_sock()
      client.data_sock.serv(0)
      client.fname = ""
      data_t = threading.Thread(target=client.print_data_t, name="data_t")
      data_t.start()
      client.cmd_sock.sendSockMessage("%s&%d\n" % (cmd,client.data_sock.get_serv_port()))
      error = 0
      while data_t.isAlive():
          r, w, e = select.select([client.cmd_sock.sock], [], [], 0.1)
          if r:
              message = client.cmd_sock.getSockMessage()
              if isinstance( message, (int, long) ):
                  client.data_sock.close()
                  client.data_sock = None
                  print >> sys.stderr, "Error reading from command socket"
              elif message.find("Error") >= 0:
                  print >> sys.stderr, message
                  client.data_sock.close()
                  client.data_sock = None
      data_t.join()
      if not error: 
        client.cmd_sock.sendSockMessage("fin\n")
        message = client.cmd_sock.getSockMessage()
        if isinstance( message, (int, long) ):
            print >> sys.stderr, "Error reading from command socket"
            exit_d = 1
            client.cmd_sock.close()
        if message.find("ack_fin") < 0:
            print >> sys.stderr, "Error in ack from server, %s" % (message)
        client.data_sock.close()
        client.data_sock = None

    elif cmd == "exit":
      exit_d = 1
      client.cmd_sock.sendSockMessage("%s\n" % (cmd))
      message = client.cmd_sock.getSockMessage()
      print "Server responded %s" % (message)
      client.cmd_sock.close()

    else:
      client.cmd_sock.sendSockMessage("%s\n" % (cmd))
      message = client.cmd_sock.getSockMessage()
      if message.find("Error") >= 0:
          print >> sys.stderr, message
      else:
          list = message.split("&")
          for l in list:
              print l


