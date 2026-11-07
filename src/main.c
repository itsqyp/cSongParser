#include <stdio.h>
#include <string.h>

#include "parser.h"
#include "decoder.h"
#include "checksum.h"



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

    // Normal audio parsing and encoding.
    if (argc != 2)
    {
        printf("\nUsage:\n");
        printf("CSongParser <audio-file>\n");
        printf("CSongParser --decode <input.sngp> <output-file>\n");
        printf("CSongParser --verify <original-file> <reconstructed-file>\n");

        return 1;
    }

    return parser_process_file(argv[1]);
}