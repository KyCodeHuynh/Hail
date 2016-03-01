#include <stdlib.h>

#include "hail.h" 

// Constructs a new Hail Packet
int // -1 on error, 0 otherwise
construct_hail_packet(
    hail_packet_t* packet,
    char seq_num, // 0-255, tracked externally
    char ack_num, 
    hail_control_code_t control, 
    char version, 
    uint64_t file_size, 
    char file_data[HAIL_CONTENT_SIZE]
)
{
    return -1;
}


// Gives the content delivered by a Hail packet
int // -1 on error (e.g., invalid packet), 0 otherwise
packet_data_hail(
    char* packet_buffer, // Pointer to received packet buffer
    char content_buffer[HAIL_CONTENT_SIZE], // Pointer to destination buffer for contents; file boundaries handled by MiniFTP
    char* seq_num // Pointer to destination for extracted sequence number
)
{
    return -1;
}
