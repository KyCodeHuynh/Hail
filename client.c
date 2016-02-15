#include <arpa/inet.h>
#include <errno.h>          // Standard error codes 
#include <netdb.h>          // getaddrinfo()
#include <netinet/in.h>     
#include <stdbool.h>
#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>     // Sockets API
#include <sys/types.h>      // Standard types
#include <unistd.h>         // Standard system calls

int main(int argc, char* argv[]) 
{
    // argc is the argument count, the length of argv[]
    // argv is the argument variable array
    // argc is at least 1, as argv[0] is the program name

    // Need at least 'hostname port filename'
    // For formatting multi-line literal strings,
    // see: http://stackoverflow.com/questions/1135841/c-multiline-string-literal
    if (argc < 4) {
        printf("\nUsage: \t%s hostname portnumber filename [OPTIONS]\n\n"
        "Send a message another endpoint using the Hail protocol.\n\n"
        "Options:\n"
        "-l L, --loss L     Simulate message loss with probability L in [0,1]\n"
        "-c C, --corrupt C  Simulate message corruption with probability C in [0,1]\n"
        "-s, --silent     Run silently without activity output to stdout or stderr\n\n", 
        argv[0]);

        return EXIT_FAILURE;
    }
    // Handle special options first, so that we can use them later
    // TODO: Use getopt() to avoid this counting madness
    // TODO: Define options booleans here
    else if (argc == 7) {
        printf("Options not yet implemented!\n");
        return EXIT_FAILURE;
    }

    // Avoid magic numbers
    const char* HAIL_SERVER = argv[1];
    const char* HAIL_PORT   = argv[2];
    const char* FILE_NAME   = argv[3];

    // 'hints' is an addrinfo that's given to 
    // getaddrinfo() to specify parameters
    struct addrinfo params; 
    memset(&params, 0, sizeof(struct addrinfo));

    // Use a UDP datagram socket type
    // 'ai' is 'addrinfo'.
    params.ai_socktype = SOCK_DGRAM;
    // IPv4 or IPv6
    params.ai_family = AF_UNSPEC;

    // argv[1] should have server name
    int status;
    struct addrinfo* results;
    // int getaddrinfo(const char *node, 
                    // const char *service, 
                    // const struct addrinfo *hints, 
                    // struct addrinfo **res);
    status = getaddrinfo(HAIL_SERVER, HAIL_PORT, &params, &results);
    if (status != 0) {
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

    // TODO: open() FILE_NAME for file and use sendto() or bind() then send()



    // Need to free up 'results'
    freeaddrinfo(results);
    close(socketFD);

    return EXIT_SUCCESS;
}