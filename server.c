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

#define HAIL_PACKET_SIZE sizeof(hail_packet_t)

typedef enum window_status {
    NOT_SENT,
    SENT,
    ACKN,
    DONE
} window_status_t;

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
    const size_t packet_size = sizeof(hail_packet_t);


    if (argc < 3) {
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
    //handle handshake
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
               
    } 

    // We now have a filename to use. open() sets read position at 0 at first.
    int file_fd = open(packet.file_data, O_RDONLY);
    struct stat fileInfo;
    // int stat(const char *pathname, struct stat *buf)
    if (stat(packet.file_data, &fileInfo) < 0) {
        fprintf(stderr, "CLIENT -- ERROR: stat() on %s failed\n", packet.file_data);
        return EXIT_FAILURE;
    }
    off_t fileSize = fileInfo.st_size;
    //char file_buffer[fileSize];
    //read(file_fd, file_buffer, fileSize);
    //write(1, file_buffer, fileSize);
    
    size_t WINDOW_LIMIT_BYTES = atoi(argv[2]);
    printf("WINDOW_LIMIT_BYTES: %d\n", WINDOW_LIMIT_BYTES );

    hail_packet_t receive_pkt;
    char recv_buffer[HAIL_PACKET_SIZE];
    memset(&receive_pkt, 0, HAIL_PACKET_SIZE);
    memset(&response_pkt, 0, HAIL_PACKET_SIZE);
    memset(recv_buffer, 0, HAIL_PACKET_SIZE);
  

    // The structure for tracking what we've sent so far, 
    // a simple buffer of previously sent packets.
    // Send up to window limit size. Use calloc() to zero-initialize allocated memory.
    size_t WINDOW_SIZE = floor(WINDOW_LIMIT_BYTES / HAIL_PACKET_SIZE);
    printf("PACKET_SIZE: %d\n", HAIL_PACKET_SIZE);
    printf("WINDOW_SIZE: %d\n", WINDOW_SIZE);
    size_t MAX_SEQ_NUM = WINDOW_SIZE*2;
    window_status_t WINDOW[MAX_SEQ_NUM];
    memset(WINDOW, NOT_SENT, sizeof(window_status_t)*MAX_SEQ_NUM);
    uint8_t bottom = 0;
    uint8_t top = WINDOW_SIZE-1;

    size_t file_offset = 0;
    size_t packets_needed = ceil(fileSize/HAIL_CONTENT_SIZE);
    size_t packets_sent = 0;
    
    char seq_num = 0; 
    char ack_num = 1;
    hail_control_code_t control = OK; 
    char version = 0;
    uint64_t file_size = fileSize;
    char file_data[HAIL_CONTENT_SIZE];

    //handle file transfer
    
    while(true){
        size_t packets_done = 0;
        //loop through window and send all packets not sent
        uint8_t i;

        

        for(i = bottom ; i != (top + 1)%MAX_SEQ_NUM; i = (i+1)%MAX_SEQ_NUM){
            printf("IN for loop\n");
            
            if(packets_sent >= packets_needed){
                break;
            }
            //if all entries are DONE
            if(WINDOW[i] == DONE){
                packets_done++;
            }

            if(packets_done == WINDOW_SIZE){
                //DONE. BREAK?
                break;
            }

            if(WINDOW[i] == NOT_SENT){
                
                lseek(file_fd, file_offset, SEEK_CUR);
                read(file_fd, file_data, HAIL_CONTENT_SIZE);
                construct_hail_packet(&response_pkt, seq_num, ack_num, control, version, file_size, file_data);

                printf("SERVER -- Sending packet %d out of %d, seq_num: %d\n", packets_sent+1, packets_needed, response_pkt.seq_num);

                if (sendto(sockfd, &response_pkt, sizeof(response_pkt), 0, (struct sockaddr *) &cli_addr, clilen ) < 0) {
                    error("SERVER -- ERROR on sending");
                }

                file_offset += HAIL_CONTENT_SIZE;
                packets_sent++;
                seq_num = (seq_num+1)%(MAX_SEQ_NUM);
                
            }
       
        }

        printf("OUT of for loop\n");

        if (recvfrom(sockfd, recv_buffer, sizeof(recv_buffer), 0, (struct sockaddr*) &cli_addr, (socklen_t*) &clilen) < 0) {
            error("SERVER -- ERROR: Receiving from client failed");
        }


        unpack_hail_packet(recv_buffer, &receive_pkt);
        printf("this is ack data: %s\n", receive_pkt.file_data);

        if((receive_pkt.control) == ACK){
            printf("SERVER -- received ACK for packet seq_num %d", receive_pkt.seq_num);
            WINDOW[receive_pkt.seq_num] = ACKN;
        }
        

        if(WINDOW[bottom] == ACKN){
            
            bottom = (bottom + 1)% WINDOW_SIZE;
            top = (top + 1)%WINDOW_SIZE;
            
            if(packets_sent >= packets_needed){
                WINDOW[top] = DONE;
            }
            else{
                WINDOW[top] = NOT_SENT;
            }
            

        }

    }

    
    free(response_buffer);
    return EXIT_SUCCESS;
}
