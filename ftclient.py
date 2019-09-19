# Name:        Christopher Teufel
# Program:     ftclient.py
# Course:      CS-372
# Assignment:  Project 02
# Last Modified: 8/12/19
#
# Description: ftclient creates a simple file transfer client that can recieve a 
#              directory listing or file from a server that is running ftserver.
#              ftclient beings by connecting to ftclient and sending other commands
#              that were entered via command line. Those commands are send as a 
#              single string. Upon reciving validation that the message is correct,
#              ftclient begins listening for a data connection on a different port,
#              that was specified in the command line. Based on the command sent, 
#              either a directory listing or a requested file is received by 
#              ftserver, which then closes the data connection. Several validation
#              messages are received including validation for correct command and that
#              the requested file exists. 
#              Socket setup is simple in python and didn't create seperate function
#
# Citation:    Relied on Standard Python Library 18.2 regarding socket programming
#              https://docs.python.org/release/2.6.5/library/socket.html
#              The program follows a similar format to examples 18.2.2
#              Looping structure taken from Piazza suggestions by instructor
#              Implimentation of minor fixes for bytearray and finding null characters
#              were found with help from stack overflow and the Standard Python Lib. 
#              Implimentation of os.path.exists from python std lib section 10.1
#              https://docs.python.org/2/library/os.path.html


#!/bin/python
import socket
import sys
import os

# function:     sendCommand
# params:       socket descriptor from command line
# pre:          ftserver is waiting for command, connection has already been 
#               initiated
# post:         ftclient returns to the main method where additionial functions 
#               are called
# returns:      1 for correct command sent or -1 for incorrect command
#
# description:  command arguments are sent as string to ftclient which then sends
#               back a validation message for a correct or incorrect command.
#
# citation:     bytearray function taken from std. python lib.
#               .join taken from other class homework
#               
def sendCommand(commandSocket):

    # first, send the server name, set to 5 bytes on both ends
    servName = socket.gethostname()[:30]
    commandSocket.send(servName + bytearray(30-len(servName)))
    
    # send command args as 30 char string, validation done on server end
    command = ' '.join(sys.argv[3:])
    command = command[:30]
    commandSocket.send(command + bytearray(30-len(command)))

    # check if server got correct command
    commandFlag = commandSocket.recv(1)
    if int(commandFlag)==1:
        print "\nCorrect command sent"
        return 1
    else:
        print "\n{}:{} received incorrect command\n".format(sys.argv[1], sys.argv[2])
        return -1

# function:     getDir
# params:       dataport specified by command line
# pre:          ftserver has recieved correct command string
# post:         ftclient has recieved and printed the directory and 
#               closed the data connection
# returns:      none
#
# description:  Creates a socket and begins listening for ftserver to connect
#               on the data port specified by the command line. 
#               Receives and prints the directory.
#               Closes the data connection
#              
# citation:     Finding null bytes adapted from stack overflow and first project
#               Server setup taken from first project and std. python lib.
#               
def getDir(dataPort):
    # setup data transfer socket
    dataSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    dataSocket.bind(("", dataPort))
    dataSocket.listen(1)
    dataCon, adr = dataSocket.accept()

    # get and print directory
    print "\nReceiving directory from {}:{}...".format(sys.argv[1][:5], sys.argv[4])
    dirBuf = dataCon.recv(1024)
    print dirBuf[:dirBuf.find('\x00')] #remove null bytes from string

# function:     getFile
# params:       dataport specified by command line, command socket for
#               file validation
# pre:          ftserver has recieved correct command string
# post:         ftclient has recieved witten the sent file and 
#               closed the data connection
# returns:      none
#
# description:  First waits on validation from ftserver that the file exists.
#               If not, the function prints a message and ends.
#               If the file is there, ftclient creates a socket and begins listening
#               for a connection from ftserver.
#               Once the connection is etablished, the ftclient receives the file in 
#               chunks and begins to write to a new file.
#               Once the file has been sent as a whole, ftserver ends the connection 
#               and the program returns to main.
#              
# citation:     Finding null bytes adapted from stack overflow and first project
#               Server setup taken from first project and std. python lib.
#               os.path.exists() taken from python std lib
#               
def getFile(commandSocket, dataPort):

    # first listen to see if the file is there
    fileExists = commandSocket.recv(1)

    if int(fileExists):
        print "\nReceiving file \'{}\' from {}:{}\n".format(sys.argv[4], sys.argv[1], dataPort)
    else:
        print "\n\'{}\' not found in directory\n".format(sys.argv[4])
        return

    # setup data transfer socket
    dataSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    dataSocket.bind(("", dataPort))
    dataSocket.listen(1)
    dataCon, adr = dataSocket.accept() 

    # make sure new file has unique name
    fn = sys.argv[4]
    if os.path.exists(fn):
        while os.path.exists(fn):
            print "\'{}\' already exists in your directory.".format(fn)
            i = fn.find('.')
            fn = fn[:i] + '_new' + fn[i:]
        print "Writing as \'{}\'\n".format(fn)

    fh = open(fn, 'w')

    # receive data in chunks as per piazza discussion, size hardcoded on each end
    while (1):        
        chunk = dataCon.recv(512)
        fh.write(chunk[:chunk.find('\x00')]) # last chunk contains null bytes
        if len(chunk)==0:
            break

    fh.close()
    dataCon.close()




# function:     main
# params:       ftserver name, ftserver port, command ('l' or '-g'),
#               file name (only with '-g'), data port all by command line
# pre:          ftserver is listening for conneciton
# post:         ftserver has closed the data connection, ftclient has
#               closed the command connection, ftserver continues to listen for
#               new connections
#               New file may be written to current directory
# returns:      new file
#
# description:  Takes arguments for the server name and port and establishes a
#               connection with ftserver. Calls sendCommand to send the command
#               to the server. Recieves validation of proper command and then calls
#               the appropriate function to create a data socket to listen and receive
#               a file or directoy listing. 
#               Closes the command connection and ends program
#              
# citation:     Socket setup taken from std python lib.
#               
if __name__ == "__main__":
    
    serverName = sys.argv[1]
    serverPort = int(sys.argv[2])


    # python sockets have simple setup, done in main
    mySocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    mySocket.connect((serverName, serverPort))

    #ends program if incorrect arguments sent
    sendCom = sendCommand(mySocket)
    if sendCom==-1:
        mySocket.close()
        sys.exit()
    
    if sys.argv[3]=="-l":
        getDir(int(sys.argv[4]))

    if sys.argv[3]=="-g":
        getFile(mySocket, int(sys.argv[5]))

    mySocket.close()