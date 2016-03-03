#include <fcntl.h> 
#include <math.h>
#include <netinet/in.h>  // Constants and structures needed for Internet domain addresses, e.g. sockaddr_in
#include <signal.h>      // Signal name macros, and the kill() prototype
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>  // Definitions of structures needed for sockets, e.g. sockaddr
#include <sys/stat.h>
#include <sys/types.h>   // Definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/wait.h>    // For the waitpid() system call
#include <unistd.h>

#include "hail.h"
#include <stdint.h>

void error(char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
    // TODO: Rename as needed
    int sockfd, 
    newsockfd, 
    portno, pid;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;
    char dgram[5000];             // Recv buffer
    uint8_t win_fst;
    uint8_t win_lst;
    const size_t packet_size = sizeof(hail_packet_t);

    if (argc < 2) {
        printf(
            "\nUsage: \t%s portnumber [OPTIONS]\n\n"
            "Begin receiving messages on portnumber.\n\n"
            "Options:\n"
            "-w W, --window -W  Change flight window size to 1 <= W packets\n"
            "-l L, --loss L     Simulate message loss with probability L in [0,1]\n"
            "-c C, --corrupt C  Simulate message corruption with probability C in [0,1]\n"
            "-s,   --silent     Run silently without activity output to stdout or stderr\n\n", 
            argv[0]
        );        

        exit(1);
    }

    // Handle special options first, so that we can use them later
    // TODO: Use getopt() to avoid this counting madness
    // TODO: Define options booleans here
    else if (argc == 6) {
        printf("Options not yet implemented!\n");
        return EXIT_FAILURE;
    }

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);	// Create socket
    if (sockfd < 0) {
        error("ERROR opening socket");
    }

    memset((char *) &serv_addr, 0, sizeof(serv_addr));	//reset memory
    // Fill in address info
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    
    printf("hello");

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        error("ERROR on binding");
    }

    // printf("right before while loop\n");
    printf("SERVER: Waiting for connections...\n");

    // Only allocate reordering buffer if it was not already created from some packet
    bool buffer_exists = false;
    char* reorder_buffer = NULL;
    hail_packet_t packet;
    memset(&packet, 0, packet_size);

    // Make sure server is always runner with infinite while loopclilen
    while (1) {
        // printf("got into while loop\n");
    	// Receive UDP from client
        clilen = sizeof(cli_addr);
    	if (recvfrom(sockfd, dgram, sizeof(dgram), 0, (struct sockaddr*) &cli_addr, (socklen_t*) &clilen) < 0) {
            error("ERROR on receiving from client");
        }

        hail_packet_t packet;
        hail_packet_t response_pkt;

        // Unpack the received message into an easy-to-use struct
        memcpy(&packet, dgram, sizeof(hail_packet_t));

        // Three-way handshake
        if (packet.control == SYN) {
            construct_hail_packet(&response_pkt, 0, 0, SYN_ACK, 0, 0, "");
            printf("SERVER: SYN ACK sent in response to ACK.\n");
        }
        else if (packet.control == ACK){
            printf("SERVER: Final ACK received from client. Connection established.\n");
        }

        // unpack packet into buffer
        char response_buffer[sizeof(hail_packet_t)];
        memcpy(response_buffer, &response_pkt, sizeof(hail_packet_t));
        memcpy(&packet, dgram, packet_size);

        // Create reordering buffer
        if (! buffer_exists) {
            uint64_t file_size = packet.file_size;
            size_t num_slots = ceil(file_size / HAIL_CONTENT_SIZE);

            reorder_buffer = (char*)malloc(num_slots * HAIL_CONTENT_SIZE);
            buffer_exists = true;
        }

        // TODO: Handle different runs of sequence numbers
        memcpy(&(reorder_buffer[(size_t)packet.seq_num]), packet.file_data, HAIL_CONTENT_SIZE);

        // printf("%s\n", dgram);
        // printf("%d\n", clilen);

        // TODO: Send ACK back to client
        // Echo input back to client 
        if (sendto(sockfd, dgram, sizeof(dgram), 0, (struct sockaddr *) &cli_addr, clilen ) < 0) {
            error("ERROR on sending");
        }
    } 
}
