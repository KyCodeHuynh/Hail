# Hail
Hail is a reliable transport-layer protocol, implemented atop UDP. 


## Protocol Design 

Hail is based upon TCP, one of the twin backbone protocols of the
modern Internet. 

At a high level, we: 

* Try to establish a connection, with a maximum number of attempts to do so.
* Break-up any data to be sent into chunks, called *segments*, then send to
  them to other host.
* The other side receives and has to reorder into a byte-stream or file 
  based on sequence numbers.
* The other side positively acknowledges received content.
* The sending side retransmits after some time-out interval for receiving an
  acknowledgement.
* One of the sides closes the connection.


### Connection Establishment 

Three-way handshake to confirm opened connection. 

Time-out interval for failure to establish connection.

TODO: Inital connect time-out. How long do we wait for trying
to establish a connection? 

TODO: Max number of initial connect attempts. How many times do 
try to establish a connection?


### Chunking into Packets

Client waits until all segments received, or hits a time-out interval 
to give up on waiting. 

TODO: How does the client know about file boundaries, i.e., when all of 
the contents of a file have been received? Does Hail ignore this and just send
a byte stream, with the client application noticing EOFs to start a new file?
Or, we could specify file lengths and send END flags. 

Received packets are put into an array (size allocated as needed or 
based on packet reporting total file size), then reordered into the final 
file before being written to disk. That means grabbing just the content portion
of each packet as it arrives and putting that into the array (copy packet
into packet struct temporarily to grab).


### Packet Format

TODO: How do we specify a field of only so many bits in C? We could do bit-
level operations to manipulate bits within a byte.

TODO: What does our sequence number specify? 

TODO: What is our maximum sequence number? How do we handle wrap-around?

TODO: What does our acknowledgement number specify?

TODO: How do we specify flags? A control code field for: 

* SYN
* SYN ACK
* ACK 
* START/END (if we have explicit file boundaries)
* CLOSE

This is all in a buffer memcpy()'ed from a struct.  

    [ Hail version: char version ]  1 byte
    [ TODO: Remainder of format  ]  TODO bytes


### Closing the Connection

TODO: How do we close the connection? A CLOSE flag in the header? We could have a set of control codes in that case. 





