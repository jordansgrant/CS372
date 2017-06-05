#!/usr/bin/python
# Program:  filecli
# Project:  CS372 Project 2
# Author:   Jordan Grant (grantjo)
# File:     filecli.py
# Function: filecli is a python program responsible for makeing a client/server 
#           connection to a remote host and setting up a file transfer session.
#
#           The client connects on a command port to the server and initiates
#           commands to change directory, get help, exit, list files, or get
#           a file. If a list or get command is issued the client listens on
#           a port and the server connects and sends the data over this
#           secondary connection. The connection is then closed and the session
#           continues.
#           
# EXTRA CREDIT
#
# 1. Sever forks off a new process for each connection so it can service
#    multiple clients
# 2. The client is interactive rather than only doing one round per call of the
#    python program. ie user inputs the commands rather than arguments
# 3. Client handles data in a separate thread
# 4. Client can change directory on the server
import os.path
import select
import sys
import socket
import threading

# max connection count for listener
conn_count = 1
# close flag for threads
close = 0

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
    # getSockStreamFile - streams data over socket to a file name fname or
    # fname incremented if collision
    def getSockStreamFile(self, fname):
      # init message, size, and size_found variables
       size = -1
       size_found = 0
       chars_recv = 0;
       wfile = None;
       # split filename from path
       head, tail = os.path.split(fname)
       #if file already exists
       if ( os.path.exists(tail) ):
           #split extension from file name
           file_n, file_e = os.path.splitext(tail)
           i = 1
           # loop until an unused incremented filename is found
           while os.path.exists("%s%d%s" % (file_n, i, file_e)):
               i += 1
           #store new incremented filename
           tail = "%s%d%s" % (file_n, i, file_e)
       #open file
       wfile = open(tail, "wb")
       # return error if failed
       if not wfile:
           return -1

       # loop until message of size is found
       while(size < 0 or chars_recv < size):
         # call recv on socket specifying the max size to recv in one call
           temp = self.sock.recv(self.recv_max)
           #if null is returned socket failed
           if temp == '':
             return -1
           #if size has not been stored yet, pull from message and store
           if not size_found:
               split_s = temp.split(" ", 1)
               size = int(split_s[0])
               temp = split_s[1]
               size_found = 1
           chars_recv += len(temp)
           #write chunk to file
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
    # save data to file thread, used for get command
    def save_data_t(self):
        server = self.data_sock.accept()
        server.getSockStreamFile(self.fname)
    # print data thread, used for ls command
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
# connect to server, throw error if failed
sock.connect(sys.argv[2], port)
# create client object
client = file_client(sock)
# clear exit flag
exit_d = 0
# loop untl exit flag set
while not exit_d :
    # get user input
    cmd = raw_input("\nEnter a command or help for list of command\n")

    if cmd == "":
      continue
    # get command requested
    elif cmd.find("get") == 0:
      # create data sock, start listening on port determined by os
      client.data_sock = file_sock()
      client.data_sock.serv(0)
      # store fname in variable
      client.fname = cmd[cmd.find(' ')+1:]
      # create thread object and start thread
      data_t = threading.Thread(target=client.save_data_t, name="data_t")
      data_t.start()
      # send command to server
      client.cmd_sock.sendSockMessage("%s&%d\n" % (cmd,client.data_sock.get_serv_port()))

      error = 0
      # while thread is active
      while data_t.isAlive():
          # check if an error message has come from the server
          r, w, e = select.select([client.cmd_sock.sock], [], [], 0.1)
          if r:
              message = client.cmd_sock.getSockMessage()
              # handle error on cmd socket
              if isinstance( message, (int, long) ):
                  client.data_sock.close()
                  client.data_sock = None
                  print >> sys.stderr, "Error reading from command socket"
              # handle error from server, close data socket
              elif message.find("Error") >= 0:
                  print >> sys.stderr, message
                  client.data_sock.close()
                  client.data_sock = None
                  error = 1
      data_t.join()
      if not error:
        # send finished message if no error
        client.cmd_sock.sendSockMessage("fin\n")
        message = client.cmd_sock.getSockMessage()
        # check for command socket error
        if isinstance( message, (int, long) ):
            print >> sys.stderr, "Error reading from command socket"
            exit_d = 1
            client.cmd_sock.close()
        # print error if server sent and error on the ack message
        elif message.find("ack_fin") < 0:
            print >> sys.stderr, "Error in ack from server, %s" % (message)
        #close data socket
        client.data_sock.close();
        client.data_sock = None
    # ls command requestd
    elif cmd == "ls":
      # create data socket, listen on port determined by os
      client.data_sock = file_sock()
      client.data_sock.serv(0)
      client.fname = ""
      # setup thread object and start thread listening
      data_t = threading.Thread(target=client.print_data_t, name="data_t")
      data_t.start()
      # send request to server
      client.cmd_sock.sendSockMessage("%s&%d\n" % (cmd,client.data_sock.get_serv_port()))
      error = 0
      # loop while data thread is alive
      while data_t.isAlive():
          #listen for errors from the server
          r, w, e = select.select([client.cmd_sock.sock], [], [], 0.1)
          if r:
              message = client.cmd_sock.getSockMessage()
              # report error on cmd socket
              if isinstance( message, (int, long) ):
                  client.data_sock.close()
                  client.data_sock = None
                  print >> sys.stderr, "Error reading from command socket"
              # report error from server
              elif message.find("Error") >= 0:
                  print >> sys.stderr, message
                  client.data_sock.close()
                  client.data_sock = None
                  error = 1
      data_t.join()
      if not error: 
        # if no error send finished message
        client.cmd_sock.sendSockMessage("fin\n")
        # get acknowledgement
        message = client.cmd_sock.getSockMessage()
        # check for error on cmd socket
        if isinstance( message, (int, long) ):
            print >> sys.stderr, "Error reading from command socket"
            exit_d = 1
            client.cmd_sock.close()
        #check for error on ack from server
        if message.find("ack_fin") < 0:
            print >> sys.stderr, "Error in ack from server, %s" % (message)
        #close data socket
        client.data_sock.close()
        client.data_sock = None
    #exit command requested
    elif cmd == "exit":
      #set exit flag
      exit_d = 1
      # alert server of the exit
      client.cmd_sock.sendSockMessage("%s\n" % (cmd))
      # get response from server
      message = client.cmd_sock.getSockMessage()
      # print response and close socket
      print "Server responded %s" % (message)
      client.cmd_sock.close()
    # other commands
    else:
      # send command to server
      client.cmd_sock.sendSockMessage("%s\n" % (cmd))
      # get response
      message = client.cmd_sock.getSockMessage()
      # check for error and print if so
      if message.find("Error") >= 0:
          print >> sys.stderr, message
      # else print list delimited by &
      else:
          list = message.split("&")
          for l in list:
              print l


