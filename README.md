# Hail and MiniFTP

Hail is a transport-layer protocol for reliable data transfer, implemented atop
UDP.

MiniFTP is a simple FTP-like protocol. We implement the client -> server
sending half of it.


## Hail Protocol Design 

Hail is based upon TCP, one of the twin backbone protocols of the
modern Internet. 

At a high level, we: 

* Try to establish a connection, with a maximum number of attempts to do so.
* Break-up any data to be sent into chunks, called *packets*, then send them
  to another host. We send up to *flight-window* size many packets at a time.
* The receiver reorders received packets within a buffer before writing the
  data back to disk as a file.
* The receiver positively acknowledges received content.
* The sender retransmits if they do not receive the acknowledgement after
  a set time-out interval.


### Segment Format

A Hail packet has the following structure:

    [ Sequence number         ]  1 byte
    [ Acknowledgement number  ]  1 byte
    [ Control code            ]  1 byte
    [ Hail version            ]  1 byte
    [ File size               ]  8 bytes 
    [ File data               ]  500 bytes

The total size of any Hail network packet is 512 bytes.
Having a power-of-2 size allows for easy alignment in memory.
There is no padding between fields or before/after the packet.
While the above is aligned on systems with a 4-byte (32-bit) word-size, 
it is not aligned for systems with a 8-byte (64-bit) word-size. 
To [prevent automatic padding](http://stackoverflow.com/questions/4306186/structure-padding-and-structure-packing), we specify that the struct be 
"packed" instead (see the `hail.h` for details).

An explanation of the field design decisions:

*TODO*: How do we handle sequence number wrap-around? When we count again from 0, we lose track of our current location. We could make our sequence number
much larger, but that necessarily increases our acknowledgement number size
to match. We introduce overhead that is unneeded for most transfers.

* Sequence number: this is the most-accessed field and so placed first. 
  It identifies the order of Hail packets as originally sent. It wraps 
  around upon hitting 255, which gives an internal limit of 256 * 500 bytes
  == 128 MB on file size before this edge case is encountered and handled.

* Acknowledgement number: a receiver echoes a sequence number as its
  acknowledgement. A sender knows to retransmit a particular packet upon not
  seeing its echoed sequence number after some time-out interval.

* Control code: our control codes mark a packet as being internal to the
  protocol's functioning. They carry no data, and their functions are explained
  in the relevant sections. Up to 256 such control codes can exist, providing
  for the standard's extensibility.

    * SYN
    * SYN ACK
    * START
    * END
    * ACK 
    * CLOSE

* Hail version: designed for graceful backwards compatibility handling, 
  the version (from 0 to 255) lets hosts decide what level of features are
  available. As Hail is still in an alpha stage, the version is kept at 0 
  for now.

* File size: this is simply an unsigned integer, 8 bytes in size to match the
  size limit of 2 EiB (exbibytes, 2^64 bits, or approximately 2.3 exabytes for
  those who prefer base-10) for individual files on most operating systems

* File data: these are the contents of a file transported by the Hail packet.
  As these are simply raw bytes, applications can apply their own higher-level 
  logic 

Note that we assume octet bytes, i.e., each is 8 bits in size. When sent, this
struct is packed into a buffer (marshalling/serialized) and unpacked
(unmarshalled/deserialized) at the destination.


### Connection Establishment 

Hail opens with a three-way handshake to negotiate a connection. At the moment,
both hosts only confirm that the version number is between 0 and 2, reserved
for the development versions of Hail. The handshake begins when a sender
sends a packet with the `SYN` code set (synchronize). The receiver replies 
with `SYN ACK` (synchronize acknowledge), the sender acknowledges with `ACK`.

The time-out interval for any of the handshake messages is 50 ms. 
A maximum of 3 retransmission attempts is made at each step. If 
any fail to acknowledge after 3 attempts with time-outs, then the 
other host is deemed unreachable.


### Chunking into Packets

The sender sends a chunk of the file at a time. It packs the Hail packets 
and fires it off. It can do so again for more of the file up until 
the *flight window* limit is hit. The flight window limits how many 
packets may be "in-flight" on the network at any one time. This allows
the sender to easily retransmit any lost or corrupted packets from the
most recent window.

Received packets are put into an array indexed by their sequence numbers 
before being written to disk. That means grabbing just the content portion
of each packet as it arrives and putting that into the array. We can 
allocate the size of the array based on the reported file size to begin. 
We'll later want to optimize this to write data to disk as it arrives, 
to avoid a heap allocation limit (which is typically around 1 MB by default). 


### Closing the Connection

Either the sender or the receiver may at any time close the
Hail connection by sending a packet with the `CLOSE` control code set.

*TODO*: What error do we give back if the client tries to send more but the server closed the connection?

*TODO*: What error do we give back if the server tries to keep responding, but the client closed the connection?


## MiniFTP Protocol Design 

MiniFTP organizes the 500 bytes of data sent within a Hail packet.


### File Format

The very first packet (with control code `START`, meaning "start file")
reserves the first 255 bytes for the file name. This matches 
[standard file-system maximums](http://stackoverflow.com/questions/6571435/limit-on-file-name-length-in-bash):

    // Within the 500 bytes of data carried by a Hail packet
    [ File name ] 255 bytes
    [ File data ] 245 bytes

For every packet after that, 500 bytes of data are sent: 

    [ File data ] 500 bytes

In both cases, we again disable automatic padding.

Because the sequence number is constantly wrapping around for larger 
files, we cannot use it as a reliable guidepost for the end of a file. 

*TODO*: How do we handle the file boundaries of large files? We could 
use the `END` control code in some way.





