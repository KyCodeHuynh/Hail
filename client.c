#include <arpa/inet.h>      // struct in_addr, inet_ntop(), etc.
#include <errno.h>          // Standard error codes 
#include <fcntl.h>          // open()
#include <netdb.h>          // getaddrinfo()
// #include <netinet/in.h>     // struct sockaddr_in and struct in_addr
#include <stdbool.h>        // 'true' and 'false' literals
#include <stdio.h>          // fprintf(), etc.
#include <stdlib.h>         // malloc(), calloc(), etc.
#include <string.h>         // strchr(), etc.
#include <sys/socket.h>     // Sockets API
#include <sys/stat.h>       // stat()
#include <sys/types.h>      // Standard types
#include <unistd.h>         // Standard system calls

#include "hail.h"

// TODO: Declare Hail helpers here

// Uncomment these header-includes if we need to 
// build and work with the raw networking structs

int main(int argc, char* argv[]) 
{
    // argc is the argument count, the length of argv[]
    // argv is the argument variable array
    // argc is at least 1, as argv[0] is the program name

    // Avoid magic numbers
    const char* HAIL_SERVER = {0};
    const char* HAIL_PORT   = {0};
    const char* FILE_NAME   = {0};

    // Need at least 'hostname port filename'
    // For formatting multi-line literal strings,
    // see: http://stackoverflow.com/questions/1135841/c-multiline-string-literal
    if (argc < 4) {
        printf(
            "\nUsage: \t%s hostname portnumber filename [OPTIONS]\n\n"
            "Send a message another endpoint using the Hail protocol.\n\n"
            "Options:\n"
            "-l L, --loss L     Simulate message loss with probability L in [0,1]\n"
            "-c C, --corrupt C  Simulate message corruption with probability C in [0,1]\n"
            "-s,   --silent     Run silently without activity output to stdout or stderr\n\n", 
            argv[0]
        );

        return EXIT_FAILURE;
    }
    // Handle special options first, so that we can use them later
    // TODO: Use getopt() to avoid this counting madness
    // TODO: Define options booleans here
    else if (argc == 7) {
        printf("Options not yet implemented!\n");
        return EXIT_FAILURE;
    }

    // We're here, so enough options were passed in
    HAIL_SERVER = argv[1];
    HAIL_PORT   = argv[2];
    FILE_NAME   = argv[3];

    // 'hints' is an addrinfo that's given to 
    // getaddrinfo() to specify parameters
    struct addrinfo params; 
    memset(&params, 0, sizeof(struct addrinfo));

    // Use a UDP datagram socket type
    // 'ai' is 'addrinfo'.
    params.ai_socktype = SOCK_DGRAM;
    // TODO: Generalize to IPv4 or IPv6
    params.ai_family = AF_INET;

    // argv[1] should have server name
    int status;
    struct addrinfo* results;
    // int getaddrinfo(const char *hostname, 
    //                 const char *servname, 
    //                 const struct addrinfo *hints, 
    //                 struct addrinfo **res);
    status = getaddrinfo(HAIL_SERVER, HAIL_PORT, &params, &results);
    if (status < 0) {
        // gai_strerror() converts error codes to messages
        // See: http://linux.die.net/man/3/gai_strerror
        fprintf(stderr, "[ERROR]: getaddrinfo() failed: %s\n", gai_strerror(status));
        return EXIT_FAILURE;
    }

    // 'results' now points to a linked-list of struct addrinfo
    // We need to find out which one of them lets us build a socket
    int socketFD = -1;
    struct addrinfo* p = NULL; 
    for (p = results; p != NULL; p = p->ai_next) {
        // int socket(int domain, int type, int protocol)
        // This is where we would have once used PF_INET
        socketFD = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (socketFD != -1) {
            // Successfully made a socket
            break;
        }
    }

    // Failed to make a working socket, so p hit end-of-list
    if (p == NULL) {
        fprintf(stderr, "[ERROR]: getaddrinfo() gave no working sockets\n");
        return EXIT_FAILURE;
    }

    // We only need the working information
    results = p;

    // Initiate three-way handshake

    // int open(const char *pathname, int flags)
    int fileDescrip = open(FILE_NAME, O_RDONLY);
    if (fileDescrip < 0) {
        fprintf(stderr, "[ERROR]: open() of %s failed\n", FILE_NAME);
        return EXIT_FAILURE;
    }

    // CERT recommends against fseek() and ftell() for determining file size
    // See: https://is.gd/mwJDph-
    struct stat fileInfo;
    // int stat(const char *pathname, struct stat *buf)
    if (stat(FILE_NAME, &fileInfo) < 0) {
        fprintf(stderr, "[ERROR]: stat() on %s failed\n", FILE_NAME);
        return EXIT_FAILURE;
    }

    // Not a regular file
    if (! S_ISREG(fileInfo.st_mode)) {
        fprintf(stderr, "[ERROR]: stat() on %s: not a regular file\n", FILE_NAME);
        return EXIT_FAILURE;
    }

    // Read file into buffer
    off_t fileSize = fileInfo.st_size;
    char* fileBuffer = (char *)malloc(sizeof(char) * fileSize);
    if (read(fileDescrip, fileBuffer, fileSize) < 0) {
        fprintf(stderr, "[ERROR]: read() of %s into buffer failed\n", FILE_NAME);
        return EXIT_FAILURE;
    }

    // Use sendto() rather than bind() + send() as this
    // is a one-time shot (for now; later we'll break up
    // the file into different chunks)

    // int sendto(int sockfd, 
    //            const void *msg, 
    //            int len, 
    //            unsigned int flags, 
    //            const struct sockaddr *to, 
    //            socklen_t tolen);
    status = sendto(socketFD, 
                    fileBuffer,
                    fileSize,
                    0,
                    results->ai_addr, // We set results = p earlier
                    results->ai_addrlen);

    if (status < 0) {
        char IP4address[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, results->ai_addr, IP4address, results->ai_addrlen);
        fprintf(stderr, "[ERROR]: sendto() %s of %s failed\n", IP4address, FILE_NAME);
        return EXIT_FAILURE;
    }

    // Need to free up 'results'
    freeaddrinfo(results);
    close(socketFD);
    free(fileBuffer);

    return EXIT_SUCCESS;
}