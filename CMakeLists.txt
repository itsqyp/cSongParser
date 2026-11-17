cmake_minimum_required(VERSION 3.20)

project(
    CSongParser
    VERSION 1.0.0
    LANGUAGES C
)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# OpenSSL configuration
set(OPENSSL_ROOT_DIR "C:/msys64/ucrt64")

find_package(OpenSSL 3.0 REQUIRED COMPONENTS SSL Crypto)

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
    src/sender.c
    src/receiver.c

    #Networking
    src/network/socket_utils.c
    src/network/tcp_server.c
    src/network/tcp_client.c
    src/network/tcp_io.c
    src/network/tcp_file_transfer.c
    src/network/tcp_chunk_transfer.c
    src/network/tcp_ack.c
    src/network/tcp_handshake.c

    #TLS
    src/network/tls_context.c
    src/network/tls_client.c
    src/network/tls_server.c
    src/network/tls_io.c
)

# ==========================================
# TLS TEST APPLICATIONS
# ==========================================

set(TLS_TEST_SOURCES
    src/network/tls_context.c
    src/network/tls_client.c
    src/network/tls_server.c
    src/network/tls_io.c
)

# TLS Test Server
add_executable(tls_test_server
    tests/tls/tls_test_server.c
    ${TLS_TEST_SOURCES}
)

target_include_directories(tls_test_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(tls_test_server PRIVATE
    ws2_32
    OpenSSL::SSL
    OpenSSL::Crypto
)

# TLS Test Client
add_executable(tls_test_client
    tests/tls/tls_test_client.c
    ${TLS_TEST_SOURCES}
)

target_include_directories(tls_test_client PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(tls_test_client PRIVATE
    ws2_32
    OpenSSL::SSL
    OpenSSL::Crypto
)

target_link_libraries(
    CSongParser
    PRIVATE
        bcrypt
        ws2_32
        OpenSSL::SSL
        OpenSSL::Crypto
)

target_include_directories(
    CSongParser
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/include
)

if(MSVC)
    target_compile_options(tls_test_server PRIVATE /W4)
    target_compile_options(tls_test_client PRIVATE /W4)
else()
    target_compile_options(tls_test_server PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(tls_test_client PRIVATE -Wall -Wextra -Wpedantic)
endif()

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