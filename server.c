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

void error(char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

int main(int argc, char* argv[])
{
    int sockfd, portno;
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

        return EXIT_FAILURE;
    }

    // Handle special options first, so that we can use them later
    // TODO: Use getopt() to avoid this counting madness
    // TODO: Define options booleans here
    else if (argc == 6) {
        error("SERVER -- Options not yet implemented!\n");
    }

    // Create socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);	
    if (sockfd < 0) {
        error("SERVER -- ERROR: Opening socket");
    }

    // Initialize to 0 to avoid junk data
    memset((char *) &serv_addr, 0, sizeof(serv_addr));

    // Fill in address info
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        error("ERROR on binding");
    }

    // printf("right before while loop\n");
    printf("SERVER -- Waiting for connections...\n");

    // Only allocate reordering buffer if it was not already created from some packet
    int status = -1;
   

    hail_packet_t packet;
    hail_packet_t response_pkt;
    char* response_buffer = (char*)malloc(packet_size);
    memset(&packet, 0, packet_size);
    memset(&response_pkt, 0, packet_size);

    // Make sure server is always runner with infinite while loopclilen
    while (true) {
        // printf("SERVER: Got into while loop\n");
    	// Receive message from client
        clilen = sizeof(cli_addr);
    	if (recvfrom(sockfd, dgram, sizeof(dgram), 0, (struct sockaddr*) &cli_addr, (socklen_t*) &clilen) < 0) {
            error("SERVER -- ERROR: Receiving from client failed");
        }

        // Unpack the received message into an easy-to-use struct
        status = unpack_hail_packet(dgram, &packet);

        // Three-way handshake
        if (packet.control == SYN) {

            printf("CLIENT -- Sent SYN to start handshake.\n"); 

            construct_hail_packet(
                &response_pkt,      // Packet pointer 
                packet.seq_num + 1, // Sequence number
                packet.seq_num,     // Acknowledgement number
                SYN_ACK,            // Control code
                0,                  // Version
                0,                  // File size 
                ""                  // File content
            );

            memcpy(response_buffer, &response_pkt, sizeof(hail_packet_t));
            // Send SYN ACK back to client
            if (sendto(sockfd, response_buffer, sizeof(response_buffer), 0, (struct sockaddr *) &cli_addr, clilen ) < 0) {
                error("SERVER -- ERROR on sending");
            }

        }
        else if (packet.control == ACK){
            printf("CLIENT -- Sent ACK in reply to SYN ACK.\n");
            printf("SERVER -- Final ACK received from client. Connection established!\n");
            
            break;
        }
        
        /*
        HANDLE FILES
           int fileDescrip = open(FILE_NAME, O_RDONLY);
            if (fileDescrip < 0) {
                fprintf(stderr, "CLIENT -- ERROR: open() of %s failed\n", FILE_NAME);
                return EXIT_FAILURE;
            }

            // CERT recommends against fseek() and ftell() for determining file size
            // See: https://is.gd/mwJDph-
            struct stat fileInfo;
            // int stat(const char *pathname, struct stat *buf)
            if (stat(FILE_NAME, &fileInfo) < 0) {
                fprintf(stderr, "CLIENT -- ERROR: stat() on %s failed\n", FILE_NAME);
                return EXIT_FAILURE;
            }

            // Not a regular file
            if (! S_ISREG(fileInfo.st_mode)) {
                fprintf(stderr, "CLIENT -- ERROR: stat() on %s: not a regular file\n", FILE_NAME);
                return EXIT_FAILURE;
            }

            // Read file into buffer
            off_t fileSize = fileInfo.st_size;
            char* fileBuffer = (char *)malloc(sizeof(char) * fileSize);
            if (read(fileDescrip, fileBuffer, fileSize) < 0) {
                fprintf(stderr, "CLIENT -- ERROR: read() of %s into buffer failed\n", FILE_NAME);
                return EXIT_FAILURE;
            }   
        */

        

        
    } 

    free(response_buffer);
    //free(reorder_buffer);
    return EXIT_SUCCESS;
}
