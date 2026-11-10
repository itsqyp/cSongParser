C Song Parser
A Modular C-Based Audio Processing and Network File Transfer System

C Song Parser is a Windows-based application written in C that combines binary audio file processing with TCP network communication.

The project focuses on understanding low-level file handling, binary data processing, custom file formats, data integrity verification, and network-based file transfer.

The application can encode an audio file into a custom binary container, divide it into smaller chunks, transfer those chunks over a TCP connection, reconstruct the original container, and extract the original audio file on the receiving end.
Current Features

1. Audio File Processing

- Binary file reading and processing.
- Audio format detection using file signatures.
- Basic audio metadata extraction.
- MP3 metadata parsing, including ID3v2 support.
- Audio duration, sample rate, bitrate, and channel detection.

2. Custom Binary Container (SNGP)

- Custom SNGP binary file format.
- SNGP version 1 and version 2 support.
- Structured binary headers.
- Original file metadata storage.
- SHA-256 payload integrity verification.
- Container validation during decoding.

3. File Chunking (SNGC)

- Splits binary containers into smaller chunks.
- Default chunk size: 1 MB.
- Custom SNGC binary chunk format.
- Chunk indexing and metadata.
- Individual chunk integrity verification.
- SNGC version 1 and version 2 support.

4. File Reassembly

- Automatic chunk discovery.
- Sequential chunk reconstruction.
- Chunk metadata validation.
- Physical file size validation.
- SHA-256 checksum verification.
- Protection against incomplete or invalid reconstruction.
- Cleanup of incomplete output files.

5. TCP Network File Transfer
   The application implements TCP-based communication using Windows Winsock.

Implemented networking features:

- TCP client and server.
- Connection establishment.
- File information handshake.
- Sender and receiver acceptance/rejection mechanism.
- Binary chunk transmission.
- Automatic chunk count discovery.
- Sequential chunk reception.
- Reliable send and receive loops.
- Transfer acknowledgment system.
- Success and failure acknowledgments.

6. Data Integrity and Validation

- SHA-256 hashing using Windows BCrypt.
- Original file integrity verification.
- SNGP payload checksum verification.
- Individual SNGC chunk checksum verification.
- File size verification after reconstruction.
- Final transfer acknowledgment.

7. Temporary File Management

- Exclusive file creation to prevent accidental overwriting.
- Cleanup of incomplete transfers.
- Cleanup of temporary chunks and containers.
- Protection of existing destination files.
- Ownership-aware cleanup to avoid deleting files that were not created by the current operation.

8.  Interactive Menu
    The application provides an interactive terminal interface.
    ====================================
    C SONG PARSER v1.0
    ====================================

           1. Send Files
           2. Receive Files
           3. Exit

Users can select whether they want to send or receive files.
How File Transfer Works
The transfer process follows this workflow:
SENDER
|
v
Select Audio File
|
v
Validate Input
|
v
Encode into SNGP
|
v
Split into SNGC
Chunks
|
v
Establish TCP
Connection
|
v
Exchange Handshake
|
v
Send Chunks
|
|
=====================
TCP CONNECTION
=====================
|
v
RECEIVER
|
v
Accept Transfer
|
v
Receive Chunks
|
v
Validate Chunks
|
v
Reassemble SNGP
|
v
Verify SHA-256 Hash
|
v
Decode Original File
|
v
Verify Output File Size
|
v
Send Final ACK
|
v
Transfer Completed

The receiver reconstructs the original audio file rather than leaving the user with internal .sngp or .sngc files.
Technologies Used
Technology Purpose
C (C17) Core application development
CMake Build configuration
Ninja Build system
MinGW GCC C compiler
Windows Winsock2 TCP networking
Windows BCrypt SHA-256 hashing
Windows API File and system operations

Project Structure
cSongParsar/
│
├── src/
│ ├── main.c
│ ├── parser.c
│ ├── format.c
│ ├── metadata.c
│ ├── encoder.c
│ ├── decoder.c
│ ├── checksum.c
│ ├── chunker.c
│ ├── reassembler.c
│ ├── sender.c
│ ├── receiver.c
│ │
│ ├── network/
│ │ ├── socket_utils.c
│ │ ├── tcp_server.c
│ │ ├── tcp_client.c
│ │ ├── tcp_io.c
│ │ ├── tcp_file_transfer.c
│ │ ├── tcp_chunk_transfer.c
│ │ ├── tcp_ack.c
│ │ └── tcp_handshake.c
│ │
│ └── ...
│
├── include/
│
├── songs/
│ └── Local audio files and test data
│
├── tests/
│
├── CMakeLists.txt
├── .gitignore
└── README.md

The structure above represents the main source organization. Individual header files are maintained alongside their corresponding modules.
Building the Project
Requirements

- Windows
- MinGW GCC
- CMake
- Ninja
- Visual Studio Code (recommended)
  Clone the Repository
  git clone https://github.com/itsqyp/cSongParsar.git

Navigate into the project:
cd cSongParsar

Configure the Project
cmake -S . -B build -G Ninja

Build
cmake --build build

Run
.\build\CSongParser.exe

Using the Application
Receiving Files

1. Launch the application.
2. Select 2. Receive Files.
3. Enter a listening port, for example 5002.
4. Wait for an incoming sender connection.
5. Review the incoming file information.
6. Accept the transfer by providing a destination file path.
7. Wait for the file to be reconstructed and verified.
   Sending Files
8. Launch the application on another terminal or computer.
9. Select 1. Send Files.
10. Enter the receiver's IP address.
11. Enter the receiver's listening port.
12. Provide the input audio file path.
13. Wait for the transfer to complete.
    For local testing, the loopback address can be used:
    127.0.0.1

For communication between separate computers, the sender must use the receiver's reachable network IP address.
Testing and Verification
The application has successfully completed an end-to-end transfer test using two application instances on the same Windows computer.

Test configuration:
Parameter Value
Input file songs/song.mp3
File size 2,445,061 bytes
Container size 2,445,269 bytes
Number of chunks 3
Chunk size 1 MB
Network protocol TCP
Test address 127.0.0.1

Successful Test Results

- TCP connection established.
- Handshake accepted.
- All three chunks transferred successfully.
- Individual chunk checksums verified.
- SNGP container reconstructed successfully.
- SNGP SHA-256 verification passed.
- Original MP3 extracted successfully.
- Output file size verified.
- Final success acknowledgment received.
  SHA-256 Verification
  Original file:
  5027DD6474AD17F87E0F99EF012DBE5843B33469866845EA35BFAACB3E9A756D

Reconstructed file:
5027DD6474AD17F87E0F99EF012DBE5843B33469866845EA35BFAACB3E9A756D

Both hashes matched exactly, confirming that the reconstructed file was byte-for-byte identical to the original.

Note: Testing between two separate physical computers is the next major validation milestone. Internet-based transfer has not yet been implemented or tested.
Current Limitations

- MP3 is currently the supported format for the complete encoding and transfer workflow.
- Networking has been tested locally using the loopback address.
- LAN communication between separate computers still needs practical testing.
- Internet-based file transfer is not implemented.
- Communication currently uses unencrypted TCP.
- Strong sender authentication has not yet been implemented.
- The application currently targets Windows.
  Future Development
  Planned improvements include:
- [ ] Test file transfer between two separate computers over a LAN.
- [ ] Expand audio format support.
- [ ] Implement encrypted network communication using TLS.
- [ ] Add sender and receiver authentication.
- [ ] Improve transfer error recovery.
- [ ] Implement transfer progress reporting.
- [ ] Improve network connection handling.
- [ ] Add a graphical user interface.
- [ ] Create a distributable Windows executable.
- [ ] Perform larger-file and network reliability testing.
      Project Goals
      The primary goal of C Song Parser is to develop a deeper understanding of:
- Low-level C programming.
- Binary file formats.
- File encoding and decoding.
- Data serialization.
- File chunking and reassembly.
- TCP socket programming.
- Network communication.
- Cryptographic hashing.
- Memory and resource management.
- Modular software architecture.
  This project is being developed incrementally, with an emphasis on modularity, validation, reliability, and understanding how each component works internally.
  License
