cmake_minimum_required(VERSION 3.20)

project(
    CSongParser
    VERSION 1.0.0
    LANGUAGES C
)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_executable(
    CSongParser
    src/main.c
    src/menu.c
    src/parser.c
    src/format.c
    src/metadata.c
    src/encoder.c
    src/decoder.c
    src/checksum.c
    src/chunker.c
    src/reassembler.c

    #Networking
    src/network/socket_utils.c
    src/network/tcp_server.c
    src/network/tcp_client.c
    src/network/tcp_io.c
    src/network/tcp_file_transfer.c
    src/network/tcp_chunk_transfer.c
    src/network/tcp_ack.c
    src/network/tcp_handshake.c
)

target_link_libraries(
    CSongParser
    PRIVATE
        bcrypt
        ws2_32
)

target_include_directories(
    CSongParser
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/include
)

if(MSVC)
    target_compile_options(
        CSongParser
        PRIVATE
            /W4
    )
else()
    target_compile_options(
        CSongParser
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
    )
endif()