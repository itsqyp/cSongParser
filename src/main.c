#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "parser.h"
#include "decoder.h"
#include "checksum.h"
#include "chunker.h"
#include "reassembler.h"
#include "network/socket_utils.h"
#include "network/tcp_server.h"
#include "network/tcp_client.h"

static int verify_files(const char *original, const char *reconstructed)
{
    unsigned char original_hash[SHA256_HASH_SIZE];
    unsigned char reconstructed_hash[SHA256_HASH_SIZE];

    printf("\n====================================\n");
    printf("        SHA-256 VERIFICATION\n");
    printf("====================================\n");

    // Calculate original file hash.
    if (checksum_sha256_file(original, original_hash) != 0)
    {
        return 1;
    }

    // Calculate reconstructed file hash.
    if (checksum_sha256_file(reconstructed, reconstructed_hash) != 0)
    {
        return 1;
    }

    printf("\nOriginal SHA-256:\n");
    checksum_print(original_hash);

    printf("\nReconstructed SHA-256:\n");
    checksum_print(reconstructed_hash);

    // Compare hashes.
    if (memcmp(original_hash, reconstructed_hash, SHA256_HASH_SIZE) == 0)
    {
        printf("\n[VERIFICATION] SUCCESS\n");
        printf("Both files are identical.\n");
    }
    else
    {
        printf("\n[VERIFICATION] FAILED\n");
        printf("The files are different.\n");
        return 1;
    }

    printf("====================================\n");

    return 0;
}



int main(int argc, char *argv[])
{
    printf("====================================\n");
    printf("         C SONG PARSER v1.0\n");
    printf("====================================\n\n");

    printf("Binary Audio Processing System\n");
    printf("Status: Initializing...\n");
    // Test Winsock initialization.
    if (argc >= 2 && strcmp(argv[1], "--net-test") == 0)
    {
    if (argc != 2)
    {
        printf("\nUsage:\n");
        printf("CSongParser --net-test\n");

        return 1;
    }

    if (network_initialize() != 0)
    {
        return 1;
    }

    network_cleanup();

    return 0;
    }
    
// Start TCP server.
if (argc >= 2 && strcmp(argv[1], "--server") == 0)
{
    if (argc != 3)
    {
        printf("\nUsage:\n");
        printf("CSongParser --server <output-prefix>\n");
        return 1;
    }

    if (network_initialize() != 0)
    {
        return 1;
    }

    int result = tcp_server_run(5000, argv[2]);

    network_cleanup();

    return result == 0 ? 0 : 1;
}
// Start TCP client.
if (strcmp(argv[1], "--client") == 0)
{
    if (argc != 4)
    {
        printf("Usage: CSongParser --client <input-prefix> <total-chunks>\n");
        return 1;
    }

    char *end_pointer;

    unsigned long total_chunks = strtoul(
        argv[3],
        &end_pointer,
        10
    );

    if (
        argv[3][0] == '\0' ||
        *end_pointer != '\0' ||
        total_chunks == 0 ||
        total_chunks > UINT32_MAX
    )
    {
        printf("[CLIENT] Invalid chunk count.\n");
        return 1;
    }

    if (network_initialize() != 0)
    {
        return 1;
    }

    int result = tcp_client_run(
        "127.0.0.1",
        5000,
        argv[2],
        (uint32_t)total_chunks
    );

    network_cleanup();

    return result == 0 ? 0 : 1;
}

    // Decode an SNGP container.
    if (argc >= 2 && strcmp(argv[1], "--decode") == 0)
    {
        if (argc != 4)
        {
            printf("\nUsage:\n");
            printf("CSongParser --decode <input.sngp> <output-file>\n");
            return 1;
        }

        return decoder_extract_payload(argv[2], argv[3]) == 0 ? 0 : 1;
    }

    // Verify two files using SHA-256.
    if (argc >= 2 && strcmp(argv[1], "--verify") == 0)
    {
        if (argc != 4)
        {
            printf("\nUsage:\n");
            printf("CSongParser --verify <original-file> <reconstructed-file>\n");
            return 1;
        }

        return verify_files(argv[2], argv[3]);
    }


    // Split an SNGP container into binary chunks.
    if (argc >= 2 && strcmp(argv[1], "--chunk") == 0)
    {
        if (argc != 5)
        {
            printf("\nUsage:\n");
            printf("CSongParser --chunk <input.sngp> <output-prefix> <chunk-size>\n");
            return 1;
        }

        char *end_pointer;

        unsigned long chunk_size = strtoul(argv[4], &end_pointer, 10);

        if (*argv[4] == '\0' ||
            *end_pointer != '\0' ||
            chunk_size == 0 ||
            chunk_size > UINT32_MAX)
        {
            printf("[CHUNKER ERROR] Invalid chunk size.\n");
            return 1;
        }

        return chunker_create_chunks(
            argv[2],
            argv[3],
            (uint32_t)chunk_size
        ) == 0 ? 0 : 1;
    }


    // Reassemble SNGC chunks into an SNGP container.
if (argc >= 2 && strcmp(argv[1], "--reassemble") == 0)
{
    if (argc != 4)
    {
        printf("\nUsage:\n");
        printf("CSongParser --reassemble <input-prefix> <output.sngp>\n");
        return 1;
    }

    return reassembler_combine_chunks(
        argv[2],
        argv[3]
    ) == 0 ? 0 : 1;
}

    // Normal audio parsing and encoding.
    if (argc != 2)
    {
        printf("\nUsage:\n");
        printf("CSongParser <audio-file>\n");
        printf("CSongParser --decode <input.sngp> <output-file>\n");
        printf("CSongParser --verify <original-file> <reconstructed-file>\n");
        printf("CSongParser --chunk <input.sngp> <output-prefix> <chunk-size>\n");
        printf("CSongParser --reassemble <input-prefix> <output.sngp>\n");
        printf("CSongParser --server <output-prefix>\n");
        printf("CSongParser --client <input-prefix> <total-chunks>\n");
        printf("CSongParser --net-test\n");
        return 1;
    }

    return parser_process_file(argv[1]);
}