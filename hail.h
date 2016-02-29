#ifndef HAIL_H 
#define HAIL_H

#define HAIL_CONTENT_SIZE 500 // 500 bytes worth of data

// Hail - Hail is a transport-layer protocol for reliable data transfer,
// implemented atop UDP

// Use an enum to have the type system enforce restrictions
typedef enum hail_control_code_t {
    SYN, 
    SYN_ACK, 
    ACK,
    START,
    END, 
    CLOSE
} hail_control_code_t;

// We have to manually disable automatic padding
// See: http://stackoverflow.com/questions/4306186/structure-padding-and-structure-packing
typedef struct __attribute__((__packed__)) hail_packet_t {
    char seq_num; // 0-255
    char ack_num; // 0-255
    hail_control_code_t control; // 0-255
    char version; // versions 0-255
    uint64_t file_size; // 8-byte file size, so files up to 2 EiB
    char file_data[HAIL_CONTENT_SIZE]; // Up to 500 bytes of file data
} hail_packet_t;

// Constructs a new Hail Packet
int // -1 on error, 0 otherwise
construct_hail_packet(
    char seq_num, // 0-255, tracked externally
    char ack_num, 
    hail_control_code_t control, 
    char version, 
    uint64_t file_size, 
    char file_data[HAIL_CONTENT_SIZE]
);

// Gives the content delivered by a Hail packet
int // -1 on error (e.g., invalid packet), 0 otherwise
packet_data_hail(
    char* packet_buffer, // Pointer to received packet buffer
    char content_buffer[HAIL_CONTENT_SIZE], // Pointer to destination buffer for contents; file boundaries handled by MiniFTP
    char* seq_num // Pointer to destination for extracted sequence number
);




#endif