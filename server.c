/* Basic web server, base code/tutorial from:
 * https://www.youtube.com/watch?v=Q1bHO4VbUck */

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sendfile.h>
#include <netinet/in.h>
#include <fcntl.h>

// nice typedef
#define saddr   struct sockaddr
#define saddrin struct sockaddr_in

// server and client addresses and IDs
saddrin serverAddr, clientAddr;
int serverId, clientId;

// server response
char response404[] = 
"HTTP/1.1 404 Not Found\n"
"Content-Type: text/html; charset=UTF-8\n\n"
// here is the default 404 page:
"<!DOCTYPE html>\n"
"<html><head><title>Error 404</title></head>\n"
"<body style='background-color:#EEFFFF'>File Not Found :(</body></html>";

char htmlHeader[] = "HTTP/1.1 200 OK\n"
                "Content-type: text/html;\n\n";

// socket helper functions
int createSocket(void);
void setServerInfo(int portNumber);
void bindServer(void);
void sendResponse(const char *fileName);

// other helper functions
int getFileSize(int fileId);
char *getRequestedFileName(const char *request);
bool isHtml(const char *fileName);

int main(int argc, char *argv[])
{
    // not really sure what this means...
    int on = 1;

    // used for storing data
    char buffer[2048];

    // make sure we were given a port arg
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        return EXIT_SUCCESS;
    }

    // open a server socket
    serverId = createSocket(); 
    
    // set server socket properties
    setsockopt(serverId, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int));
    // argument is the port #
    setServerInfo(atoi(argv[1]));

    // bind the server
    bindServer();

    // listen
    listen(serverId, 10);

    // main listen loop
    for(;;)
    {
        int clientSize = sizeof(clientAddr);
        clientId = accept(serverId, (saddr *)&clientAddr, 
                &clientSize);
        if (clientId == -1) {
            fprintf(stderr, "Failed to accept client\n");
            continue;
        }

        // spawn a process for that connection
        if (!fork())
        {
            // child process
            close(serverId);
            memset(buffer, 0, sizeof(buffer));

            // read in what the client sent
            read(clientId, buffer, sizeof(buffer)-1);
            printf("%s\n", buffer);

            // see if an empty file request was given
            if (!strncmp(buffer, "GET / ", 6)) {
                printf("No file specified! Trying index.html\n");

                sendResponse("index.html");
            }

            else {
                char *fileName = getRequestedFileName(buffer);
                printf("fileName: %s\n", fileName);

                sendResponse(fileName);
            }

            // close the client
            close(clientId);

            // kill the fork
            exit(EXIT_SUCCESS);
        }
        // parent process
        close(clientId);
    }

    return 0;
}

int createSocket(void)
{
    int id = socket(AF_INET, SOCK_STREAM, 0);
    if (id < 0) {
        fprintf(stderr, "Error opening socket\n");
        exit(EXIT_FAILURE);
    }

    return id;
}

void setServerInfo(int portNumber)
{
    // init the server info to zeros
    memset((char *)&serverAddr, 0, sizeof(serverAddr));

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(portNumber);
}

void bindServer(void)
{
    int bindResult = bind(serverId, (saddr *)&serverAddr, 
            sizeof(serverAddr));
    if (bindResult < 0) {
        fprintf(stderr, "Error binding!\n");
        exit(EXIT_FAILURE);
    }
}

void sendResponse(const char *fileName)
{
    // try and open the file
    int id = open(fileName, O_RDONLY);

    // failed to open
    if (id == -1) {
        printf("Couldn't find or open %s...\n", fileName);
        
        // send the default server response
        write(clientId, response404, sizeof(response404) - 1);
    }

    // successful open
    else {
        // get the size of our file
        int fileSize = getFileSize(id);
        printf("File Size: %d\n", fileSize);

        // send the file to our client
        if (isHtml(fileName)) {
            write(clientId, htmlHeader, strlen(htmlHeader));
        }
        sendfile(clientId, id, NULL, fileSize);

        // close the file
        close(id);
    }
}

int getFileSize(int fileId)
{
    // move to the beginning of the file
    lseek(fileId, 0, SEEK_SET);

    // the number of bytes by storing the offset
    // to the end of the file
    int size = lseek(fileId, 0, SEEK_END);

    // move back to the beginning of the file
    lseek(fileId, 0, SEEK_SET);

    // return the value
    return size;
}

char *getRequestedFileName(const char *request)
{
    static char requestedFile[200];
    sscanf(request, "GET /%s HTTP", requestedFile);

    printf("Requested file: %s\n", requestedFile);

    return requestedFile;
}

bool isHtml(const char *fileName)
{
    bool result;
    char *ext = strrchr(fileName, '.');
    if (!ext) {
        result = false;
    }
    else {
        result = !strcmp(ext+1, "html");
    }

    return result;
}



