#include <arpa/inet.h>      // struct in_addr, inet_ntop(), etc.
#include <errno.h>          // Standard error codes 
#include <fcntl.h>          // open()
#include <math.h>
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

// Uncomment these header-includes if we need to 
// build and work with the raw networking structs

int main(int argc, char* argv[]) 
{
    // argc is the argument count, the length of argv[]
    // argv is the argument variable array
    // argc is at least 1, as argv[0] is the program name

    // Avoid magic numbers. TODO: Do these need atoi() and htons()?
    const char* HAIL_SERVER = {0};
    const char* HAIL_PORT   = {0};

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

    unsigned int filename_length = fmin(255, strlen(argv[3]));
    char* FILE_NAME = (char *)malloc(256 * sizeof(char));
    memcpy(FILE_NAME, argv[3], filename_length);
    FILE_NAME[filename_length + 1] = '\0';

    fprintf(stderr, "CLIENT -- File name from argv[3]: \"%s\"\n", FILE_NAME);

    // 'hints' is an addrinfo that's given to 
    // getaddrinfo() to specify parameters
    struct addrinfo params; 
    memset(&params, 0, sizeof(struct addrinfo));

    // Use a UDP datagram socket type
    // 'ai' is 'addrinfo'.
    params.ai_socktype = SOCK_DGRAM;
    params.ai_family = AF_INET;

    // argv[1] should have server name
    int status;
    struct addrinfo* results;
    bool buffer_exists = false;
    char* reorder_buffer = NULL;
    // int getaddrinfo(const char *hostname, 
    //                 const char *servname, 
    //                 const struct addrinfo *hints, 
    //                 struct addrinfo **res);
    status = getaddrinfo(HAIL_SERVER, HAIL_PORT, &params, &results);
    if (status < 0) {
        // gai_strerror() converts error codes to messages
        // See: http://linux.die.net/man/3/gai_strerror
        fprintf(stderr, "CLIENT -- ERROR: getaddrinfo() failed: %s\n", gai_strerror(status));
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
        fprintf(stderr, "CLIENT -- ERROR: getaddrinfo() gave no working sockets\n");
        return EXIT_FAILURE;
    }

    // We only need the working information
    results = p;

 
    // Outside of loop, to avoid creating multiple packets
    // We'd lose track of old ones whenever we overwrote addresses otherwise
    hail_packet_t* packet = (hail_packet_t*)malloc(sizeof(hail_packet_t));
    const size_t packet_size = sizeof(hail_packet_t);

    // For now at least, we'll ignore the wasted data space on handshake packets
    char file_data[HAIL_CONTENT_SIZE]; 
    memset(file_data, 0, HAIL_CONTENT_SIZE);

    // sendto() expects a buffer, not a struct
    char* send_buffer = (char*)malloc(packet_size);
    memcpy(send_buffer, packet, packet_size);

    // For later receiving of packets
    char* recv_buffer = (char*)malloc(packet_size);
    memset(recv_buffer, 0, packet_size);

    char seq_num = 0;
    char ack_num = 0;
    hail_control_code_t control = SYN;
    char version = 0;
    uint64_t file_size = 0;

    // Loop until a three-way handshake has been established
    while (true) {
        // Use sendto() rather than bind() + send() as this
        // is a one-time shot (for now; later we'll break up
        // the file into different chunks)
        status = construct_hail_packet(
            packet,    // Packet pointer
            seq_num,   // Sequence number
            ack_num,   // Acknowledgement number
            control,   // Control code
            version,   // Version
            file_size, // File size
            file_data  // File data
        );

        if (status < 0) {
            fprintf(stderr, "construct_hail_packet() failed! [Line: %d]\n", __LINE__);
        }

        if (sendto(
            socketFD,              // int sockfd
            packet,                // const void* msg
            packet_size,           // int len
            0,                     // unsigned int flags
            results->ai_addr,      // const struct sockaddr* to; we set results = p earlier
            results->ai_addrlen    // socklen_t tolen

            ) < 0) {

            char IP4address[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, results->ai_addr, IP4address, results->ai_addrlen);
            fprintf(stderr, "CLIENT -- ERROR: SYN sendto() %s of %s failed\n", IP4address, FILE_NAME);

            return EXIT_FAILURE;
        }

        // Received SYN ACK?
        // Filled by recvfrom(), as OS finds out source address
        // from headers in packets
        struct sockaddr server_addr;
        socklen_t server_addr_len = sizeof(struct sockaddr);
        int bytes_received = 0;

        bytes_received = recvfrom(
            socketFD,        // int sockfd; same socket for some connection
            recv_buffer,     // void* buf
            packet_size,     // int len
            0,               // unsigned int flags
            &server_addr,    // Filled by recvfrom(), as OS finds out source address
            &server_addr_len // from headers in packets
        );

        if (bytes_received == 0) {
            fprintf(stderr, "CLIENT -- No bytes received. Assuming connection closed.\n");
        }
        else if (bytes_received < 0) {
            fprintf(stderr, "CLIENT -- ERROR: recvfrom() failed. Line %d\n", __LINE__);
        }

        hail_packet_t recv_packet;
        memset(&recv_packet, 0, packet_size);
        
        // Unpack to see if it's a SYN ACK
        if ( unpack_hail_packet(recv_buffer, &recv_packet) < 0) {
            fprintf(stderr, "unpack_hail_packet() failed! [Line: %d]\n", __LINE__);
        }

        // Server SYN ACK received; send final ACK
        if (recv_packet.control == SYN_ACK) {
            // fprintf(stderr, "CLIENT -- Entered SYN_ACK if statement.\n");
            printf("SERVER -- SYN ACK sent in response to SYN.\n");

            // Filename + nullbyte
            memcpy(file_data, FILE_NAME, filename_length + 1);
            fprintf(stderr, "CLIENT -- File name in send buffer: \"%s\"\n", file_data);

            // Create and send back final ACK
            status = construct_hail_packet(
                packet,   
                recv_packet.seq_num + 1,  
                recv_packet.seq_num,  
                ACK,  
                0,  
                file_size,
                file_data
            );

            // printf("%s\n", packet->file_data);
            if (sendto(socketFD, packet, packet_size,0, results->ai_addr, results->ai_addrlen) < 0) {
                
                char IP4address[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, results->ai_addr, IP4address, results->ai_addrlen);
                fprintf(stderr, "CLIENT -- ERROR: ACK sendto() %s of %s failed\n", IP4address, FILE_NAME);

                return EXIT_FAILURE;
            }



        }
        else{
            printf("%s\n", recv_buffer);
        }



        // Create reordering buffer
        /*if (! buffer_exists) {
            uint64_t file_size = packet.file_size;
            size_t num_slots = ceil(file_size / HAIL_CONTENT_SIZE);

            reorder_buffer = (char*)malloc(num_slots * HAIL_CONTENT_SIZE);
            buffer_exists = true;
        }

        // TODO: Handle different runs of sequence numbers
        memcpy(&(reorder_buffer[(size_t)packet.seq_num]), packet.file_data, HAIL_CONTENT_SIZE);*/
    }
    

    // TODO: Send file in chunks. Update curPos!
    // Keep track of where we are within the buffer
    // size_t curPos = 0;

    // Need to free up 'results' and everything else
    freeaddrinfo(results);
    close(socketFD);
    free(FILE_NAME);
    free(packet);
    free(send_buffer);
    free(recv_buffer);

    return EXIT_SUCCESS;
}