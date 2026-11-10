# C Song Parser

A Windows command-line tool written in C that encodes an audio file into a custom binary container, sends it over TCP in verified chunks, and restores the original file on the receiver.

## Features

- **Audio parsing**: detects format by file signature; reads MP3 metadata (ID3v2, duration, sample rate, bitrate, channels).
- **SNGP container** (v1, v2): structured header, original file metadata, SHA-256 payload checksum, validation on decode.
- **SNGC chunking** (v1, v2): splits a container into 1 MB chunks, each with an index and its own SHA-256 checksum.
- **Reassembly**: discovers chunks, validates metadata, checks size and hash, deletes incomplete output.
- **TCP transfer** (Winsock): client/server, file-info handshake, accept/reject by receiver, chunk streaming, final success/failure ACK.
- **Safe file handling**: exclusive file creation, no overwriting of existing files, cleanup limited to files created by the current operation.

## Transfer Flow

```
Sender                                   Receiver
------                                   --------
Validate audio file
Encode to SNGP
Split into SNGC chunks
Handshake (file info)  ------------->    Accept / reject
Send chunks            ------------->    Verify each chunk
                                         Reassemble SNGP, verify SHA-256
                                         Decode original file, verify size
Receive final ACK      <-------------    Send ACK
```

The receiver ends up with the original audio file, not the intermediate `.sngp` / `.sngc` files.

## Build

Requirements: Windows, MinGW GCC, CMake, Ninja.

```
git clone https://github.com/itsqyp/cSongParser.git
cd cSongParsar
cmake -S . -B build -G Ninja
cmake --build build
.\build\CSongParser.exe
```

## Usage

```
C SONG PARSER v1.0
  1. Send Files
  2. Receive Files
  3. Exit
```

**Receive**: choose `2`, enter a listening port (e.g. `5002`), review the incoming file info, enter a destination path to accept.

**Send**: choose `1`, enter the receiver's IP and port, enter the audio file path. Use `127.0.0.1` for local testing.

## Verified Result

End-to-end transfer between two local instances over `127.0.0.1`:

| Item                               | Value                                                              |
| ---------------------------------- | ------------------------------------------------------------------ |
| Input                              | `songs/song.mp3` (2,445,061 bytes)                                 |
| Container                          | 2,445,269 bytes                                                    |
| Chunks                             | 3 × up to 1 MB                                                     |
| Chunk, container, and file hashes  | All verified                                                       |
| SHA-256 (original = reconstructed) | `5027DD6474AD17F87E0F99EF012DBE5843B33469866845EA35BFAACB3E9A756D` |

## Limitations

- Full encode/transfer workflow supports MP3 only.
- Tested on loopback only; LAN and internet transfer are untested.
- No encryption (plain TCP) and no sender authentication.
- Windows only.

## Roadmap

- LAN testing between two machines
- Additional audio formats
- TLS and authentication
- Transfer progress and error recovery
- GUI and distributable executable

## Tech Stack

C17, CMake, Ninja, MinGW GCC, Winsock2, Windows BCrypt (SHA-256), Windows API.
