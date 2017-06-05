### COMPILATION INSTRUCTIONS
* fileserve

Compile using the following command:

```
make
```

You can clean up with the following command:

```
make clean
```

* filecli.py

Use the following command to make it executable:

```
chmod u+x filecli.py
```

### USAGE Instructions


* fileserve

usage: ./fileserve port

* filecli.py

usage: ./filecli.py port host

Start the server running first then connect to it with chatcli.py. You will be
prompted in chatcli for a command. To learn the syntax you can issue the "help"
command and the server will respond with the available commands. 

cd <path> - this will allow you to change directories. ~ or other bash
            shortcuts will not work
help      - issue command list

exit      - exits the client

ls        - get a directory listing over secondary data socket

get <file> - retrieves the file with the name passed in by file


### TESTING

Testing was done on the flip server and on my personal machine (macbook pro late 2011 running ubuntu 16.04)

### EXTRA CREDIT

1. Sever forks off a new process for each connection so it can service
    multiple clients
2. The client is interactive rather than only doing one round per call of the
    python program. ie user inputs the commands rather than arguments
3. Client handles data in a separate thread
4. Client can change directory on the server
5. Client increments filenames when there are collistions
