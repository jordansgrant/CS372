### COMPILATION INSTRUCTIONS
* chatclient

Compile using the following command:

```
make
```

You can clean up with the following command:

```
make clean
```

* chatserve.py

Use the following command to make it executable:

```
chmod u+x chatserve.py
```

### USAGE Instructions

Both programs can act as a client or server. In either usage the first argument
of port number is required. If only the port is given the programs will act as
a server.  If a hostname is specied, they will function as a client.

* chatclient

usage: ./chatclient port [host]

* chatserve.py

usage: ./chatserve.py port [host]

_Both programs will prompt the user for their handle (name that will be displayed to their peer) prior to initiating any connections_

The connection is terminated by a '\quit' message from either peer

Upon a \quit message the following will happen:

* chatclient will terminate
* chatserve will prepare to accept another connection

_chatserv will not handle multiple connections as it won't call accept again until the send and receive threads join with the main thread_

### TESTING

Testing was done on the flip server and on my personal machine (macbook pro late 2011 running ubuntu 16.04)

### EXTRA CREDIT

1. Program was developed so that either chatclient or chatserve can make the
   initial communication
2. Both hosts may send at anytime without having to trade off
3. Execution of the sending and receiving is done in sepearate threads
4. The screen is split using ncurses library so that the user input and
   responses are in separate panels
5. Color is used to give contrast between the handle prompts and the messages
