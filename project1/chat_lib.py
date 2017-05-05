import curses
import socket

class sock_thread:
	def __init__(self, handle):
		self.handle = handle
		self.sock_fd
		self.win
		self.close
		
	def do_thread(self):
		raise NotImplementedError("Abstract class, must implement in child class")
		
	def prompt(self, color):
		curses.attron(COLOR_PAIR(color))
		curses.waddstr(self.win, self.handle)
		curses.attroff(COLOR_PAIR(color))
		
class chat_recv(sock_thread):
	def do_thread(self):
		while !self.close:
			
		
		