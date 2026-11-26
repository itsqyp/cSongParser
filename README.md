# C Song Parser

A Windows command-line tool written in C that encodes an audio file into a custom binary container, transfers it over an encrypted TLS connection using TCP, and restores the original file on the receiver.

## Features

- **Audio parsing**: detects format by file signature; reads MP3 metadata (ID3v2, duration, sample rate, bitrate, channels).
- **SNGP container (v1, v2)**: structured header, original file metadata, SHA-256 payload checksum, validation on decode.
- **SNGC chunking (v1, v2)**: splits a container into 1 MB chunks, each with an index and its own SHA-256 checksum.
- **Reassembly**: discovers chunks, validates metadata, checks size and hash, deletes incomplete output.
- **TCP transfer (Winsock)**: client/server, file-info handshake, accept/reject by receiver, chunk streaming, final success/failure ACK.
- **TLS encryption**: encrypts data in transit using OpenSSL; supports TLS 1.2 and TLS 1.3.
- **Server certificate verification**: sender verifies the receiver's certificate against a trusted certificate and validates the receiver IP address.
- **Safe file handling**: exclusive file creation, no overwriting of existing files, cleanup limited to files created by the current operation.

## Transfer Flow

```text
Sender                                      Receiver
------                                      --------
Validate audio file
Encode to SNGP
Split into SNGC chunks
Connect over TCP       ------------------>
                       <------------------  TLS handshake
Verify receiver certificate
Complete TLS handshake
                       <------------------  Secure connection established
Handshake (file info)  ------------------>  Accept / reject
Send chunks            ------------------>  Verify each chunk
                                            Reassemble SNGP, verify SHA-256
                                            Decode original file, verify size
Receive final ACK      <------------------  Send ACK
Close TLS connection
```

The receiver ends up with the original audio file, not the intermediate `.sngp` / `.sngc` files.

## Build

Requirements: Windows, MinGW GCC, CMake, Ninja, OpenSSL 3.0 or later.

OpenSSL must be installed and available to CMake. This project currently uses the MSYS2 UCRT64 OpenSSL installation.

```powershell
git clone https://github.com/itsqyp/cSongParser.git
cd cSongParsar

cmake -S . -B build -G Ninja
cmake --build build

.\build\CSongParser.exe
```

Before running the application, make sure the OpenSSL DLL directory is available in your PATH:

```powershell
$env:Path = "C:\msys64\ucrt64\bin;$env:Path"
```

The receiver requires its certificate and private key:

```text
certs/
└── receiver/
    ├── receiver-cert.pem
    └── receiver-key.pem
```

The sender requires the trusted receiver certificate:

```text
certs/
└── sender/
    └── trusted-receiver-cert.pem
```

The receiver's private key must be kept secure and must never be committed to the repository.

## Usage

```text
C SONG PARSER v1.0
   1. Send Files
   2. Receive Files
   3. Exit
```

**Receive**: choose `2`, enter a listening port (e.g. `5002`), review the incoming file information, and enter a destination file path to accept the transfer.

**Send**: choose `1`, enter the receiver's IP and port, and enter the audio file path. Use `127.0.0.1` for local testing.

The sender verifies the receiver's TLS certificate before transferring file information or audio data.

## Verified Result

**Complete end-to-end TLS file transfer between two local instances over `127.0.0.1`:**

| Item                              | Result                             |
| --------------------------------- | ---------------------------------- |
| TLS protocol                      | TLS 1.3                            |
| Cipher suite                      | TLS_AES_256_GCM_SHA384             |
| Receiver certificate verification | Successful                         |
| Invalid certificate test          | Correctly rejected                 |
| File information handshake        | Successful                         |
| Transfer acceptance               | Successful                         |
| Chunks transferred                | 3                                  |
| Chunk reception                   | All 3 chunks received successfully |
| Chunk checksums                   | All verified                       |
| SNGP reassembly                   | Successful                         |
| SNGP payload SHA-256 verification | Successful                         |
| Original MP3 extraction           | Successful                         |
| Received file size                | 2,445,061 bytes                    |
| Original and received MP3 SHA-256 | Identical                          |

**File integrity verification:**

| Item                          | Value                                                              |
| ----------------------------- | ------------------------------------------------------------------ |
| Original file                 | `songs/song.mp3`                                                   |
| Received file                 | `songs/output/received-song.mp3`                                   |
| Original file size            | 2,445,061 bytes                                                    |
| Container size                | 2,445,269 bytes                                                    |
| Number of chunks              | 3                                                                  |
| SHA-256 (original = received) | `5027DD6474AD17F87E0F99EF012DBE5843B33469866845EA35BFAACB3E9A756D` |

The complete transfer pipeline has been successfully tested over an encrypted TLS connection. The original and received MP3 files produced identical SHA-256 hashes, confirming bit-for-bit file integrity.

## Limitations

- Full encode/transfer workflow supports MP3 only.
- Tested on loopback only; LAN and internet transfer are untested.
- Receiver authentication is implemented through TLS certificate verification; sender identity is not authenticated.
- Receiver certificate provisioning and trust must be configured manually.
- Windows only.

## Roadmap

- LAN testing between two machines
- Additional audio formats
- Sender authentication
- Transfer progress and error recovery
- GUI and distributable executable

## Tech Stack

C17, CMake, Ninja, MinGW GCC, Winsock2, Windows BCrypt (SHA-256), OpenSSL 3, TLS 1.2/1.3, Windows API.
