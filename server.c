/*
This program is essentially an HTTP server. A new socket is created and the program begins
listening on the port 8000, it accepts a connection request from a client, interprets the request,
and serves a 200 OK if the requested data could be found & accessed, and a 404 Not Found if not.
*/

#include <stdio.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <pthread.h>
#define MAX 200

// a structure for storing parameters to be passed as an argument when creating a new thread
struct ParamStruct {
    int desc;
    int pathlength;
    char defaultpath[MAX];
};

// function to process incoming requests from client connections
void* respondToRequest(void* params) {
    // local declaration of params structure
    struct ParamStruct *myparams = (struct ParamStruct*)params;
    char requestBuffer[MAX]; // declare request buffer
    bzero(requestBuffer, MAX); // fill buffer with zeros
    // read request to requestBuffer
    read(myparams->desc, requestBuffer, sizeof(requestBuffer));
    char type[MAX], path[MAX], protocol[MAX];
    printf("Request received: ");
    sscanf(requestBuffer, "%s %s %s", type, path, protocol); // assign variables (path is important)
    char fullpath[myparams->pathlength + strlen(path)]; // new var to append request path to server path
    strcpy(fullpath, myparams->defaultpath); // copies default server path to new fullpath var
    strcat(fullpath, path); // concatenates the requested path to the defdault server path
    printf("%s %s %s\n", type, path, protocol); // print the request to console cause why not
    fflush(stdout);
    struct stat sb;
    int s = stat(fullpath, &sb); // obtain descriptor of requested path
    // if stat returned -1 then nothing was found at the specified path -> 404
    if (s == -1) {
        char errMsg[43] = "HTTP/1.0 404 Not Found\nContent-Length: 13\n\n"; // hard-coded 404 message
        write(myparams->desc, errMsg, sizeof(errMsg)); // respond to client with 404 message
    }
    // if requested path is a regular file
    if (S_ISREG(sb.st_mode)) {
        // src = requested file's descriptor
        int src = open(fullpath, O_RDONLY);
        // if src returns -1 then the file cant be read
        if (src == -1) {
	    printf("Error: could not read '%s'\n", fullpath);
	} else {
            // char array the size of the requested file for memory efficiency (;
            char filebuffer[sb.st_size];
            // char array the size of the file + 35 bytes dedicated to the HTTP response
            char fullresponse[sb.st_size + 34];
            // hard-coded HTTP response message
            strcpy(fullresponse, "HTTP/1.0 200 OK\nContent-Length 6\n\n");
            // read from file to filebuffer
            read(src, filebuffer, sb.st_size);
            // concatenate fullresponse with file contents
            strcat(fullresponse, filebuffer);
            // respond to client with fullresponse
            write(myparams->desc, fullresponse, sizeof(fullresponse));
        }
    }
    // succesfully sent a response
    printf("Response sent!\n");
    fflush(stdout);
    // close client connect
    close(myparams->desc);
}

// main function takes in a path for the server to theoretically run on
int main(int argc, char *argv[]) {
    // if no path is specified send usage message
    if (argc != 2) {
        printf("Usage: './server <PATH>'\n");
        return 1;
    }
    // declare main path char array
    char mainpath[strlen(argv[1])];
    // copy path without leading '/' to mainpath
    memcpy(mainpath, &argv[1][1], strlen(argv[1])-1);
    // append terminating null
    mainpath[strlen(argv[1])-1] = '\0';
    // declare descriptors for the socket and client
    int socketDesc, clientDesc;
    // socket address structures for the server and client
    struct sockaddr_in serv, cli;
    // creating the socket with IPv4 and using the TCP protocol
    socketDesc = socket(AF_INET, SOCK_STREAM, 0);
    // if socket() returns -1 send error message
    if (socketDesc == -1) {
        printf("Error: Could not create new socket\n");
        return 1;
    }
    // fill server socket address with zeros
    bzero(&serv, sizeof(serv));
    // assign the socket an IP and PORT (8000 in this case)
    serv.sin_family = AF_INET;
    serv.sin_port = htons(8000);
    serv.sin_addr.s_addr = htonl(INADDR_ANY);

    // attempt to bind the socket to assigned IP and PORT, error message on failure
    if ((bind(socketDesc, (struct sockaddr*)&serv, sizeof(serv))) == -1) {
        printf("Error: Could not bind to specified address and port\n");
        return 1;
    }
    // check to make sure server can listen to the socket, error message on failure
    if ((listen(socketDesc, 5)) == -1) {
        printf("Error: Could not listen to socket\n");
        return 1;
    }
    // store size of client address structure
    int clientSize = sizeof(cli);
    printf("Now listening...\n");
    fflush(stdout);
    // infinitely listen for client connections
    for (;;) {
        // try to accept an incoming connection, error message on failure
        clientDesc = accept(socketDesc, (struct sockaddr*)&cli, &clientSize);
        if (clientDesc == -1) {
            printf("Error: Could not accept packets from client\n");
            fflush(stdout);
            return 1;
        }
        printf("Connection established...\n");
        fflush(stdout);
        // params structure to be passed an an argument in creating a new thread
        struct ParamStruct params;
        // client descriptor to be passed
        params.desc = clientDesc;
        // length of default path (mainpath) to be passed
        params.pathlength = strlen(mainpath);
        // default path (mainpath) itself to be passed
        strcpy(params.defaultpath, mainpath);
        // thread id
        pthread_t id;
        // create a new thread to process a request made by the new client connection
        // runs respondToRequest() function with params struct as an argument
        pthread_create(&id, NULL, respondToRequest, &params);
    }
    // close the socket
    close(socketDesc);
}
