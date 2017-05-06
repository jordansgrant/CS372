#!/usr/bin/python
import sys
import curses
import socket
import threading

conn_count = 1
close = 0

ALT_BACKSPACE = 127
ALT_ENTER = 10


class chat_sock(object):
	def __init__(self, sock=None):
		if sock is None:
			self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		else:
			self.sock = sock

		self.recv_max = 2048

	def connect(self, host, port):
		self.sock.connect((host, port))

	def serv(self, port):
		self.sock.bind(("0.0.0.0", port))
		self.sock.listen(conn_count)

	def close(self):
		self.sock.shutdown(socket.SHUT_RDWR)
		self.sock.close()

	def accept(self):
		(client_raw, address) = self.sock.accept()
		client = chat_sock(client_raw)
		return client

	def sendSockMessage(self, msg):
		size = len(msg)
		sent = 0
		while sent < size:
			retval = self.sock.send(msg[sent:])
			if retval == 0:
				return -1
			sent += retval
		return sent

	def getSockMessage(self):
		message = ''
		while(message.find("#") < 0):
			temp = self.sock.recv(self.recv_max)
			if temp == '':
				return -1
			message = message + temp;
		return message.replace("#", "")


class chat_recv(object):
	def __init__(self, name, handle):
		self.handle = handle
		self.name = name
		self.sock = ""
		self.win = ""

	def prompt(self, handle, color):
		self.win.addstr(handle, curses.color_pair(color))
		self.win.addstr("> ", curses.color_pair(color))
		self.win.refresh()

	def do_thread(self):
		global close
		while not close:
			msg = self.sock.getSockMessage()
			if (msg < 0):
				close = 1
				self.sock.close()
				break
			elif(msg == "\\quit"):
				self.win.addstr("\rConnection was closed by peer\n")
				self.win.refresh()
				close = 1
				self.sock.close()
				break
			handle = msg.split('>')
			self.prompt(handle[0], 2)
			self.win.addstr(handle[1])
			self.win.addch('\n')
			self.win.refresh()

class chat_send(object):
	def __init__(self, name, handle):
		self.handle = handle
		self.name = name
		self.sock = ""
		self.win = ""

	def prompt(self, handle, color):
		self.win.addstr(handle, curses.color_pair(color))
		self.win.addstr("> ", curses.color_pair(color))
		self.win.refresh()

	def do_thread(self):
		global close
		self.prompt(self.handle, 1)
		buffer = ''
		while not close:
			ch = self.win.getch()
			if ch != curses.ERR:
				if ch == curses.KEY_DC or ch == curses.KEY_BACKSPACE or ch == ALT_BACKSPACE:
                                    if len(buffer) > 0:
                                        buffer = buffer[:-1]
                                        (y,x) = self.win.getyx()
                                        self.win.delch(y, x-1)
                                        self.win.refresh()
			   	elif ch == curses.KEY_ENTER or ch == ALT_ENTER:
				   if buffer == "\\quit":
					   buffer = buffer + "#"
					   self.sock.sendSockMessage(buffer)
					   close = 1
					   self.sock.close()
					   break
				   elif len(buffer) > 0:
					   buffer = buffer + "#"
					   retval = self.sock.sendSockMessage(self.handle + ">" + buffer)
					   if (retval < 0):
						   self.win.addstr("Failed to write to socket :(\n")
						   self.win.refresh()
						   close = 1
						   self.sock.close()
						   break
					   buffer = ''
				   self.win.addch('\n')
				   self.prompt(self.handle, 1)
				   self.win.refresh()
			   	else:
				   if (ch > 31 and ch < 127):
					   buffer = buffer + chr(ch)
					   self.win.addch(ch)
					   self.win.refresh()


serv = 1

if len(sys.argv) < 2:
    print >> sys.stderr, "usage: %s port [host]" % (sys.argv[0])
    sys.exit(1)
if not sys.argv[1].isdigit():
    print >> sys.stderr, "usage: %s port [host]" % (sys.argv[0])
    sys.exit(1)

elif len(sys.argv) > 2:
    serv = 0



handle = raw_input("What's your handle?\n");


port = int(sys.argv[1]);

if (serv):
    sock = chat_sock()
    sock.serv(port)

    while 1:
        close = 0
        print "Waiting for a new connection with a chatclient"
        client = sock.accept()

        recv = chat_recv("recv_thread",handle)
        send = chat_send("send_thread",handle)

        recv.sock = client
        send.sock = client

        retval = client.getSockMessage()
        if (retval < 0):
            client.close()
            print >> stderr, "Failed to read from socket\n"
            continue

        window = curses.initscr()
        curses.cbreak()
        curses.noecho()
        curses.start_color()

        curses.init_pair(1, curses.COLOR_GREEN, curses.COLOR_BLACK)
        curses.init_pair(2, curses.COLOR_CYAN, curses.COLOR_BLACK)
        curses.init_pair(3, curses.COLOR_YELLOW, curses.COLOR_BLACK)

        lines = (curses.LINES - 1) / 2
        send.win = window.subwin(lines, curses.COLS-1, 0, 0)
        recv.win = window.subwin(lines, curses.COLS-1, lines+1, 0)
        send.win.nodelay(1)
        send.win.keypad(1)
        send.win.scrollok(1)

        recv.win.scrollok(1)
        recv.win.addstr("You are now connected to " + retval + "\n", curses.color_pair(3))

        send.win.refresh()
        recv.win.refresh()

        send_t = threading.Thread(target=send.do_thread, name="send_thread")
        recv_t = threading.Thread(target=recv.do_thread, name="recv_thread")

        send_t.start()
        recv_t.start()

        send_t.join()
        recv_t.join()

        send.win.clear();
        recv.win.clear();
        send.win.refresh();
        recv.win.refresh();
        curses.endwin();

else:
    sock = chat_sock()
    sock.connect(sys.argv[2], port)

    sock.sendSockMessage(handle+"#")

    recv = chat_recv("recv_thread",handle)
    send = chat_send("send_thread",handle)

    recv.sock = sock
    send.sock = sock

    window = curses.initscr()
    curses.cbreak()
    curses.noecho()
    curses.start_color()
    curses.init_pair(1, curses.COLOR_GREEN, curses.COLOR_BLACK)
    curses.init_pair(2, curses.COLOR_CYAN, curses.COLOR_BLACK)
    curses.init_pair(3, curses.COLOR_YELLOW, curses.COLOR_BLACK)

    lines = (curses.LINES - 1) / 2
    send.win = window.subwin(lines, curses.COLS-1, 0, 0)
    recv.win = window.subwin(lines, curses.COLS-1, lines+1, 0)
    send.win.nodelay(1)
    send.win.keypad(1)
    send.win.scrollok(1)

    recv.win.scrollok(1)
    send.win.refresh()
    recv.win.refresh()

    send_t = threading.Thread(target=send.do_thread, name="send_thread")
    recv_t = threading.Thread(target=recv.do_thread, name="recv_thread")

    send_t.start()
    recv_t.start()

    send_t.join()
    recv_t.join()

curses.endwin();
