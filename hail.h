#ifndef HAIL_H 
#define HAIL_H

// Hail - Hail is a transport-layer protocol for reliable data transfer,
// implemented atop UDP

// We have to manually disable automatic padding
// See: http://stackoverflow.com/questions/4306186/structure-padding-and-structure-packing
typedef struct hail_packet_t {
    char seq_num; // 0-255
    char ack_num; // 0-255
    char control; // 0-255
    char version; // versions 0-255
    char file_size[8]; // Up to 2 EiB
    char file_data[500]; // Up to 500 bytes of file data
} hail_packet_t;

#endif