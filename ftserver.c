/*
 * Name:        Christopher Teufel
 * Program:     ftserver.c
 * Course:      CS-372
 * Assignment:  Project 02
 * Last Modified: 8/12/19
 * Description: ftserver creates a file transfer application server that can send
 *              a file or directory listing to a client running ftclient.py. 
 *              The server opens and begins listening for a client connection and 
 *              command. Once the command is received, the server validates the command
 *              is correct and connects to the client's data connection to send the directory
 *              or file, and closes the data connection.
 *              Once the connection is closed, the server continues to listen.
 *              Multiple validation messages are sent back to the client to signal
 *              it to continue. 
 *              The server can be stopped by entering Ctrl-C on the command line.
 * 
 * Citaion:     Relied on Beej's Guide to Socket Programming found at
 *              https://beej.us/guide/bgnet/html/single/bgnet.html
 *              As per piazza discussion, code was used directly from the guide
 *              to develop the program.
 *              Specifically, section 5 was relied upon the most
 *              Additionally, help for minor fixes such as removing newline chars
 *              from user input was found from stack overflow
 */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <assert.h>
#include <dirent.h>

/* Function:    serverSetup
 * params:      string pointer port number
 * returns:     command socket fd as integer
 * pre:         port is not in use
 * post:        server is setup to listen for incoming connections
 * 
 * description: gets address info, then socket, binds, then listens
 * 
 * Citaion:     Taken from Beejs Guide, section 5
 */
int serverSetup(char* portNum){

    struct addrinfo hints, *res;
    int addrStatus, sockfd, bindStatus, listenStatus;

    // !! don't forget your error checking for these calls !!

    // first, load up address structs with getaddrinfo():

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;  // use IPv4 or IPv6, whichever
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;     // fill in my IP for me

    addrStatus = getaddrinfo(NULL, portNum, &hints, &res);
    if (addrStatus != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(addrStatus));
        exit(1);
    }

    // make a socket, bind it, and listen on it:

    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd==-1){ 
        fprintf(stderr, "socket() error \n");
        exit(1);
    }
    bindStatus = bind(sockfd, res->ai_addr, res->ai_addrlen);
    if (bindStatus==-1){ 
        fprintf(stderr, "bind() error \n");
        exit(1);
    }
    listenStatus = listen(sockfd, 1);
    if(listenStatus==-1){
        fprintf(stderr, "listen() error \n");
        exit(1);
    } else{
        printf("Now listening on port %s\n", portNum);
    }

    return sockfd;
}

/* Function:    clientSetup
 * params:      string pointer server name, str pointer port number
 * returns:     data socket fd as integer
 * pre:         server is already listening on specified port
 * post:        server is setup to listen for incoming connections
 * 
 * description: gets address info, then socket, then connects to 
 *              the 'server' running on ftclient
 * 
 * Citaion:     Taken from Beejs Guide, section 5
 */
int clientSetup(char* servName, char* portNum){
    int status;
    struct addrinfo hints;
    struct addrinfo *servinfo;
    memset(&hints, 0, sizeof hints); 
    hints.ai_family = AF_UNSPEC;     
    hints.ai_socktype = SOCK_STREAM; 

    // get ready to connect
    status = getaddrinfo(servName, portNum, &hints, &servinfo);
    if (status != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }

    int s = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if (s==-1){ 
        fprintf(stderr, "socket() error: \n");
        exit(1);
    }
    
    status = connect(s, servinfo->ai_addr, servinfo->ai_addrlen);
    if (status==-1){ 
        fprintf(stderr, "connect() error: \n");
        exit(1);
    }
    return s;
}

/* Function:    sendDir
 * params:      full client name, data transfer port, client handle for display, all
 *              as char*
 * returns: none
 * pre:         'server' on client is already listening, dataPort is unused, 'server' 
 *              on client is waiting for the directory as a single transfer
 * 
 * post:        ftclient gets the directory
 * 
 * description: gets the directory as a single string and sends to ftclient on the 
 *              data transfer socket.
 *              First connects to the 'server' on ftclient by calling clientSetup
 * 
 * Citaion:     Socket and send taken from Beejs Guide, section 5
 *              Directory code adapted from GNU manual:
 *              https://www.gnu.org/software/libc/manual/html_node/Simple-Directory-Lister.html
 * 
 * Calls:       clientSetup to get socket for data transfer
 */
void sendDir(char* clientName, char* dataPort, char* clientHandle){
    int socket = clientSetup(clientName, dataPort);
    char fileNameBuf[1024];
    memset(fileNameBuf, 0, sizeof(fileNameBuf));
    DIR *dp;
    struct dirent *ep;
    dp = opendir(".");
    printf("Sending directory to %s:%s\n", clientHandle, dataPort);
    if (dp){
        while ((ep = readdir(dp)) != NULL){
            // remove dots from directory listing
            if(strcmp(ep->d_name, ".")!=0 && strcmp(ep->d_name, "..")!=0){
                strcat(fileNameBuf, ep->d_name);
                strcat(fileNameBuf, "\n");
            }
        }
        closedir(dp);
        
        send(socket, fileNameBuf, sizeof(fileNameBuf), 0);
        close(socket);
    }
}

/* Function:    sendFile
 * params:      int command socket
 *              file name, full client name, data transfer port, client handle for display, all
 *              as char*
 * returns:     none
 * pre:         'server' on client is already listening, dataPort is unused, 'server' on client is
 *              waiting for the file chunks
 * 
 * post:        ftclient get the file if it exists
 * 
 * description: First checks to see if the file exists, similar to directory code from sendDir.
 *              Then will either send 1 or 0 on the command socket to ftclient to let it know if 
 *              the file is there.
 *              If not, the program ends without initiationg datatransfer connections
 *              If it is, reads the file chunk by chunk, sends each chunk in a while loop until the
 *              end of the file is reached.
 *              Can continuously send chunks since ftclient is continuously recieving until the 
 *              data connection is closed.
 * 
 * Citaion:     Socket and send taken from Beejs Guide, section 5
 *              Directory code adapted from GNU manual:
 *              https://www.gnu.org/software/libc/manual/html_node/Simple-Directory-Lister.html
 *              Looping structure taken from Piazza suggestions by instructor.
 * 
 * Calls:       clientSetup to get socket for data transfer
 */
void sendFile(int comSock, char* fileName, char* clientName, char* dataPort, char* clientHandle){

    // search for the file
    printf("Searching for file\n");
    int foundFlag = 0;
    int foundStatus;
    DIR *dp;
    struct dirent *ep;
    dp = opendir(".");
    if (dp){
        while ((ep = readdir(dp)) != NULL){
            if (strcmp(fileName, ep->d_name)==0){
                foundFlag=1;
                break;
            }
        }
        closedir(dp);        
    }
    
    // send flag to ftclient
    if(foundFlag){
        foundStatus = send(comSock, "1", 1, 0);
        if(foundStatus==-1){
            fprintf(stderr, "error sending file found validation\n");
            exit(1);
        } 
    } else {
        foundStatus = send(comSock, "0", 1, 0);
        if(foundStatus==-1){
            fprintf(stderr, "error sending file found validation\n");
            exit(1);
        }
        printf("Requested file \'%s\' not found\n", fileName);
        // end function early without connecting to datasocket if file not found
        return;
    }

    // setup datatransfer socket
    int socket = clientSetup(clientName, dataPort);
    printf("Sending file \'%s\' to %s:%s\n", fileName, clientHandle, dataPort);

    // read file into chunks of 512 bytes
    char readBuf[512];
    FILE* fh = fopen(fileName, "r");
    int bR, fileChunkStatus;
    while(1){
        memset(readBuf, 0, sizeof(readBuf));
        bR = fread(readBuf, 1, 512, fh);
        if(bR<=0){
            printf("EOF\n");
            break;
        }

        // sends chunk to ftclient
        fileChunkStatus = send(socket, readBuf, sizeof(readBuf), 0);
    }

    // closing socket sends empty string, signaling to ftclient to end connection
    close(socket);
    fclose(fh);

}

/* Function:    parseCommand
 * params:      int command socket  
 *              command string,  clientName,  full clientName, command port all as 
 *              all as char*
 * 
 * returns:     none
 * 
 * pre:         command connection already exists, command string has been received
 * 
 * post:        ftclient gets parsed validation messages, command is parsed and correct function called.
 * 
 * description: The command string is parsed to see if a proper command has been sent. If it is 
 *              correct, a validation message, '1', is sent to ftclient and correct function is called.
 *              If the command string is not valid, '0' is sent to ftclient. 
 * 
 * Citaion:     Send taken from Beejs Guide, section 5
 * 
 * Calls:       sendFile and sendDir
 */
void parseCommand(int socket, char* comStr, char* clientName, char* fullClientName, char* commandPort){
    int validCommandStatus;
    char* dataPort;
    char* com = strtok(comStr, " ");

    if(strcmp(com, "-l")==0){
        dataPort = strtok(NULL, " ");
        printf("List directory requested from %s:%s\n", clientName, commandPort);
        validCommandStatus = send(socket, "1", 1, 0);
        if(validCommandStatus==-1){
            fprintf(stderr, "error sending command validation\n");
            exit(1);
        } 
        sendDir(fullClientName, dataPort, clientName);
    } 
    else if(strcmp(com, "-g")==0){
        char* fileName = strtok(NULL, " ");
        dataPort = strtok(NULL, " ");
        printf("%s requested file \'%s\' on port %s\n", clientName, fileName, dataPort);
        validCommandStatus = send(socket, "1", 1, 0);
        if(validCommandStatus==-1){
            fprintf(stderr, "error sending command validation\n");
            exit(1);
        } 
        sendFile(socket, fileName, fullClientName, dataPort, clientName);
    } 
    else{
        printf("Invalid command sent by client\n");
        validCommandStatus = send(socket, "0", 1, 0);
        if(validCommandStatus==-1){
            fprintf(stderr, "error sending command validation\n");
            exit(1);
        } 
    }

}

/* Function:    getCommand
 * params:      int command socket, char* command Port  
 * 
 * returns:     none
 * 
 * pre:         command connection already initiated, command string has been sent by ftclient
 * 
 * post:        command string is sent to parseCommand
 * 
 * description: First reveives the clients full server name and then the command string. Sends the command
 *              string to parseCommand
 * 
 * Citaion:     Recv taken from Beejs Guide, section 5.1
 * 
 * Calls:       parseCommand
 */
void getCommand(int socket, char* commandPort){

    // first print clients name
    char clientFullName[31];
    char clientName[6];
    int recSNStatus = recv(socket, clientFullName, 30, 0);
    if (recSNStatus==-1){
        fprintf(stderr, "client handle recv error\n");
        exit(1);
    } else {
        strncpy(clientName, clientFullName, 5);
        printf("connection from %s\n", clientName);
    }

    // receive command string
    char commandStr[30];
    int comStrStatus = recv(socket, commandStr, 30, 0);
    if (comStrStatus==-1){
        fprintf(stderr, "error receiving command string \n");
        exit(1);
    } else {
        // parse command string and perform other calls
        parseCommand(socket, commandStr, clientName, clientFullName, commandPort);
    }
}

/* Program:     main method
 * params:      command port  
 * 
 * returns:     none
 * 
 * pre:         port is unoccupied
 * 
 * post:        server begins listening for connections from ftclient
 * 
 * description: Sets up the server to listen, accepts new connection, begins
 *              the process of responding to the server
 * 
 * Citaion:     Socket functions taken from Beejs Guide, section 5
 * 
 * Calls:       serverSetup, getCommand
 */
int main(int argc, char *argv[]){ 

    // taken from Beej's guide section 5.1
    struct sockaddr_storage their_addr;
    socklen_t addr_size;
    int sockfd = serverSetup(argv[1]);
    addr_size = sizeof their_addr;
    int new_fd;

    // continuously listens for new requests
    while(1){
        printf("\n");

        // accept performed multiple times here, needs to be seperated from serverSetup
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);
        if(new_fd==-1){
            fprintf(stderr, "accept() error\n");
            exit(1);
        } else{
            getCommand(new_fd, argv[1]);
        }
    }
    close(sockfd);
    printf("Control socket closed for business\n You probably won't see this...");
}