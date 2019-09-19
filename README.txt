Christopher Teufel
teufelc@oregonstate.edu
8/12/2019
CS-372
Project 02

FILES:
ftserver.c
ftclient.py
makefile 

DESCRIPTION:
ftserver.c is used to create the server and send files or directory
ftclient.py is used to create the client, send command to ftserver, and
receive the directory or files
makefile is only used for ftserver.c

INSTRUCTIONS:
'make server' compiles the server program.
'./server [port]' runs the server program.
'Ctrl+C' entered on the command line ends the server.
'make clean' removes the chat output file

'python ftclient.py [server name] [server port] [command] [data port]' runs the program.
To get the directory listing, enter:
    'python ftclient.py [server name] [server port] -l [data port]'
This should be able to list up to ~100 files.
To get a file, enter:
    'python ftclient.py [server name] [server port] -g [filename] [data port]'
Be sure to include an extension for the filename.
The server will validate that the file exists on the server.
If it exists in the client directory, it will be written as filename_new.txt
