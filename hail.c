#include <stdlib.h>
#include <stdint.h>
#include <string.h>
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
    memcpy(packet->seq_num, &seq_num, 1);
    memcpy(packet->ack_num, &ack_num, 1);
    memcpy(packet->control, &control, 1);
    memcpy(packet->version, &version, 1);
    memcpy(packet->file_size, &file_size, 8);
    memcpy(packet->file_data, &file_data, 500);

    return 0;
}


// Gives the content delivered by a Hail packet
int // -1 on error (e.g., invalid packet), 0 otherwise
unpack_hail_packet(
    char* packet_buffer, // Pointer to received packet buffer
    //char content_buffer[HAIL_CONTENT_SIZE], // Pointer to destination buffer for contents; file boundaries handled by MiniFTP
    hail_packet_t* packet
)
{
    memcpy(packet, packet_buffer, sizeof(hail_packet_t));

    return 0;
}
