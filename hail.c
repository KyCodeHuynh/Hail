#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "hail.h" 

// Constructs a new Hail Packet
int // -1 on error, 0 otherwise
construct_hail_packet(
    char seq_num, // 0-255, tracked externally
    char ack_num, 
    hail_control_code_t control, 
    char version, 
    uint64_t file_size, 
    char file_data[HAIL_CONTENT_SIZE],
    hail_packet_t newPacket
)
{
    memcpy(&newPacket.seq_num, &seq_num, 1);
    memcpy(&newPacket.ack_num, &ack_num, 1);
    memcpy(&newPacket.control, &control, 1);
    memcpy(&newPacket.version, &version, 1);
    memcpy(&newPacket.file_size, &file_size, 8);
    memcpy(&newPacket.file_data, &file_data, 500);
    return 0;
}


// Gives the content delivered by a Hail packet
int // -1 on error (e.g., invalid packet), 0 otherwise
packet_data_hail(
    char* packet_buffer, // Pointer to received packet buffer
    //char content_buffer[HAIL_CONTENT_SIZE], // Pointer to destination buffer for contents; file boundaries handled by MiniFTP
    hail_packet_t newPacket,
    char* seq_num // Pointer to destination for extracted sequence number
)
{
    memcpy(&newPacket, packet_buffer, sizeof(packet_buffer));
    memcpy(seq_num, &newPacket.seq_num, 1);
    return 0;
}
