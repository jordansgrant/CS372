#!/usr/bin/python
import sys
import curses
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
# members: sock 	- the actual socket connection
#		 : recv_max - the maximum size buffer to receive
class chat_sock(object):
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
		self.sock.bind(("0.0.0.0", port))
		# start listening on socket
		self.sock.listen(conn_count)
	# close, end the socket connection
	def close(self):
		self.sock.shutdown(socket.SHUT_RDWR)
		self.sock.close()
	#accept, accepts a connection to a remote client
	#wraps returned socket in a chat_sock and returns
	def accept(self):
		(client_raw, address) = self.sock.accept()
		client = chat_sock(client_raw)
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

# chat_recv is a class that handles displaying messages received to the user
# it defines a do_thread method meant execute in its own thread
class chat_recv(object):
	# constructor initializes member variables used during execution
	def __init__(self, handle, sock):
		self.handle = handle
		self.sock = sock
		self.win = ""
	# prompt, prints the prompt for a message in color
	def prompt(self, handle, color):
		self.win.addstr(handle, curses.color_pair(color))
		self.win.addstr("> ", curses.color_pair(color))
		self.win.refresh()
	# do thread, main entry point for the receive thread
	def do_thread(self):
		# specify close flag as a global so close will effect send thread as well
		global close
		# loop until the close flag is set
		while not close:
			# get the full message from the socket
			msg = self.sock.getSockMessage()
			# close the connection and set the close flag if error occurs
			if (msg < 0):
				close = 1
				self.sock.close()
				break
			# break the connection if the remote user sends a quit message
			elif(msg == "\\quit"):
				close = 1	# set the close flag
				self.sock.close()
				break
			# split handle from message
			handle = msg.split('>')
			# write a colored prompt to screen
			self.prompt(handle[0], 2)
			# write message to screen in standard color
			self.win.addstr(handle[1])
			self.win.addch('\n')
			self.win.refresh()

# chat_send is a class that handles the displaying and sending of messages to peer
# it defines a do_thread method meant execute in its own thread
class chat_send(object):
	# constructor initializes member variables
	def __init__(self, handle, sock):
		self.handle = handle
		self.sock = sock
		self.win = ""
	# prompt, prints the prompt for a message in color
	def prompt(self, handle, color):
		self.win.addstr(handle, curses.color_pair(color))
		self.win.addstr("> ", curses.color_pair(color))
		self.win.refresh()
	# do thread, main entry point for the send thread
	def do_thread(self):
		# declare close as global for the thread so setting it will also
		# effect the recv thread
		global close
		# give a color prompt
		self.prompt(self.handle, 1)
		# declare the message buffer
		buffer = ''
		# loop until close is set
		while not close:
			# get a character of input or ERR if no input given
			ch = self.win.getch()
			# if character was received from keyboard
			if ch != curses.ERR:
				# check if it was a backspace
				if ch == curses.KEY_DC or ch == curses.KEY_BACKSPACE or ch == ALT_BACKSPACE:
									# remove char from buffer if buffer isnt empty
                                    if len(buffer) > 0:
										# remove char from buffer
                                        buffer = buffer[:-1]
										# get cursor position
                                        (y,x) = self.win.getyx()
										# remove char from screen
                                        self.win.delch(y, x-1)
                                        self.win.refresh()
				# if an enter was received send a message (if there is one),
				# clear buffer, and reprompt
			   	elif ch == curses.KEY_ENTER or ch == ALT_ENTER:
					# if quit is received
				   if buffer == "\\quit":
					   # get quit message ready to send to peer
					   buffer = buffer + "\n"
					   # send quit message to peer
					   self.sock.sendSockMessage(buffer)
					   close = 1			# set close flag
					   self.sock.close()	# close socket
					   break
				   # if buffer isnt empty
				   elif len(buffer) > 0:
					   # add sigil to buffer and send with handle prepended
					   buffer = buffer + "\n"
					   retval = self.sock.sendSockMessage(self.handle + ">" + buffer)
					   # if there is a socket error close connection
					   if (retval < 0):
						   self.win.addstr("Failed to write to socket :(\n")
						   self.win.refresh()
						   close = 1			# set close flag
						   self.sock.close()
						   break
					   # reset buffer
					   buffer = ''
				   # reprompt user with color
				   self.win.addch('\n')
				   self.prompt(self.handle, 1)
				   self.win.refresh()
			   	else:
					# if printable character is found add it to buffer
				   if (ch > 31 and ch < 127):
					   # add char to buffer
					   buffer = buffer + chr(ch)
					   # display on screen
					   self.win.addch(ch)
					   self.win.refresh()

# set server flag
serv = 1

# if too few arguments are give display usage message
if len(sys.argv) < 2:
    print >> sys.stderr, "usage: %s port [host]" % (sys.argv[0])
    sys.exit(1)
# if first argument is not a number give usage message
if not sys.argv[1].isdigit():
    print >> sys.stderr, "usage: %s port [host]" % (sys.argv[0])
    sys.exit(1)
# if second argument is present attempt a client connection
elif len(sys.argv) > 2:
    serv = 0

# get user's handle
handle = raw_input("What's your handle?\n");

# convert port to an integer
port = int(sys.argv[1]);

# if a server connection was specified
if (serv):
	# create a new socket
    sock = chat_sock()
	# start listening on port
    sock.serv(port)
	# loop forever, until an interupt signal is received
    while 1:
		# sec close flag
        close = 0
		# give a explanatory message to user
        print "Waiting for a new connection with a chatclient"
		# wait for a connection
        client = sock.accept()
		# create the recv and send classes
        recv = chat_recv(handle, client)
        send = chat_send(handle, client)
		# wait for initial message from client
        retval = client.getSockMessage()
		# end connection on error
        if (retval < 0):
            client.close()
            print >> stderr, "Failed to read from socket\n"
            continue
		# initialize main curses window
        window = curses.initscr()
        curses.cbreak()				# each char of input should be avail, not just after newline
        curses.noecho()				# user input is not echo'd to screen
        curses.start_color()		# enable color
		# intitialize 3 different color pairs
        curses.init_pair(1, curses.COLOR_GREEN, curses.COLOR_BLACK)
        curses.init_pair(2, curses.COLOR_CYAN, curses.COLOR_BLACK)
        curses.init_pair(3, curses.COLOR_YELLOW, curses.COLOR_BLACK)
		# get the height of each sub-window
        lines = (curses.LINES - 1) / 2
		# initialize send window to the top half of the console
        send.win = window.subwin(lines, curses.COLS-1, 0, 0)
		# initilialize recv window to the bottom half
        recv.win = window.subwin(lines, curses.COLS-1, lines+1, 0)
		# getch should return immediatley even if no input
        send.win.nodelay(1)
		# special keys should be returned by getch
        send.win.keypad(1)
		# window should scroll when trying to type passed the last lines
        send.win.scrollok(1)
        recv.win.scrollok(1)
		# give a message to host that hes connected
        recv.win.addstr("You are now connected to " + retval + "\n", curses.color_pair(3))
		# refresh windows
        send.win.refresh()
        recv.win.refresh()
		# initialize threads: target is the entry point
        send_t = threading.Thread(target=send.do_thread, name="send_thread")
        recv_t = threading.Thread(target=recv.do_thread, name="recv_thread")
		# start the threads
        send_t.start()
        recv_t.start()
		# wait for them to terminate
        send_t.join()
        recv_t.join()
		# clear content from window
        send.win.clear();
        recv.win.clear();
        send.win.refresh();
        recv.win.refresh();
		# end curses session
        curses.endwin();
# attempt a client connection
else:
	# create socket and attempt connection, throw exception on error
    sock = chat_sock()
    sock.connect(sys.argv[2], port)
	# send initial message with users handle to server
    sock.sendSockMessage(handle+"\n")
	# initialize thread classes
    recv = chat_recv(handle, sock)
    send = chat_send(handle, sock)
    # initialize main curses window
    window = curses.initscr()
    curses.cbreak()				# each char of input should be avail, not just after newline
    curses.noecho()				# user input is not echo'd to screen
    curses.start_color()		# enable color
    # intitialize 2 different color pairs
    curses.init_pair(1, curses.COLOR_GREEN, curses.COLOR_BLACK)
    curses.init_pair(2, curses.COLOR_CYAN, curses.COLOR_BLACK)
    # get the height of each sub-window
    lines = (curses.LINES - 1) / 2
    # initialize send window to the top half of the console
    send.win = window.subwin(lines, curses.COLS-1, 0, 0)
    # initilialize recv window to the bottom half
    recv.win = window.subwin(lines, curses.COLS-1, lines+1, 0)
    # getch should return immediatley even if no input
    send.win.nodelay(1)
    # special keys should be returned by getch
    send.win.keypad(1)
    # window should scroll when trying to type passed the last lines
    send.win.scrollok(1)
    recv.win.scrollok(1)
    # initialize threads: target is the entry point
    send_t = threading.Thread(target=send.do_thread, name="send_thread")
    recv_t = threading.Thread(target=recv.do_thread, name="recv_thread")
    # start threads
    send_t.start()
    recv_t.start()
    # wait for threads to terminate
    send_t.join()
    recv_t.join()
# reset console
curses.endwin();
