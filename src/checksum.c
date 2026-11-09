#include <stdio.h>
#include <stdint.h>
#include <windows.h>
#include <bcrypt.h>

#include "checksum.h"

#define BUFFER_SIZE 4096

// Internal function: calculate SHA-256 from the current file position.
static int checksum_sha256_stream(
    FILE *file,
    uint64_t length,
    int limited,
    unsigned char hash[SHA256_HASH_SIZE]
)
{
    BCRYPT_ALG_HANDLE algorithm = NULL;
    BCRYPT_HASH_HANDLE hash_handle = NULL;

    NTSTATUS status;

    unsigned char buffer[BUFFER_SIZE];

    DWORD object_size = 0;
    DWORD hash_size = 0;
    DWORD result_size = 0;

    unsigned char *hash_object = NULL;

    uint64_t remaining = length;

    // Initialize SHA-256.
    status = BCryptOpenAlgorithmProvider(
        &algorithm,
        BCRYPT_SHA256_ALGORITHM,
        NULL,
        0
    );

    if (status != 0)
    {
        printf("[CHECKSUM ERROR] Could not initialize SHA-256.\n");
        goto cleanup;
    }

    // Get hash object size.
    status = BCryptGetProperty(
        algorithm,
        BCRYPT_OBJECT_LENGTH,
        (PUCHAR)&object_size,
        sizeof(object_size),
        &result_size,
        0
    );

    if (status != 0)
    {
        printf("[CHECKSUM ERROR] Could not get hash object size.\n");
        goto cleanup;
    }

    // Get SHA-256 output size.
    status = BCryptGetProperty(
        algorithm,
        BCRYPT_HASH_LENGTH,
        (PUCHAR)&hash_size,
        sizeof(hash_size),
        &result_size,
        0
    );

    if (status != 0 || hash_size != SHA256_HASH_SIZE)
    {
        printf("[CHECKSUM ERROR] Invalid SHA-256 hash size.\n");
        goto cleanup;
    }

    // Allocate hash object memory.
    hash_object = (unsigned char *)HeapAlloc(
        GetProcessHeap(),
        0,
        object_size
    );

    if (hash_object == NULL)
    {
        printf("[CHECKSUM ERROR] Memory allocation failed.\n");
        goto cleanup;
    }

    // Create hash handle.
    status = BCryptCreateHash(
        algorithm,
        &hash_handle,
        hash_object,
        object_size,
        NULL,
        0,
        0
    );

    if (status != 0)
    {
        printf("[CHECKSUM ERROR] Could not create hash.\n");
        goto cleanup;
    }

    // Read and hash file data incrementally.
    while (!limited || remaining > 0)
    {
        size_t bytes_to_read = BUFFER_SIZE;

        if (limited && remaining < bytes_to_read)
        {
            bytes_to_read = (size_t)remaining;
        }

        size_t bytes_read = fread(
            buffer,
            1,
            bytes_to_read,
            file
        );

        if (bytes_read == 0)
        {
            if (ferror(file))
            {
                printf("[CHECKSUM ERROR] File reading failed.\n");
                goto cleanup;
            }

            if (limited && remaining > 0)
            {
                printf("[CHECKSUM ERROR] Unexpected end of file.\n");
                goto cleanup;
            }

            break;
        }

        status = BCryptHashData(
            hash_handle,
            buffer,
            (ULONG)bytes_read,
            0
        );

        if (status != 0)
        {
            printf("[CHECKSUM ERROR] Failed to update hash.\n");
            goto cleanup;
        }

        if (limited)
        {
            remaining -= bytes_read;
        }
    }

    // Finalize SHA-256.
    status = BCryptFinishHash(
        hash_handle,
        hash,
        SHA256_HASH_SIZE,
        0
    );

    if (status != 0)
    {
        printf("[CHECKSUM ERROR] Failed to finalize hash.\n");
        goto cleanup;
    }

    // Cleanup.
    BCryptDestroyHash(hash_handle);
    BCryptCloseAlgorithmProvider(algorithm, 0);
    HeapFree(GetProcessHeap(), 0, hash_object);

    return 0;

cleanup:

    if (hash_handle != NULL)
        BCryptDestroyHash(hash_handle);

    if (algorithm != NULL)
        BCryptCloseAlgorithmProvider(algorithm, 0);

    if (hash_object != NULL)
        HeapFree(GetProcessHeap(), 0, hash_object);

    return -1;
}


// Calculate SHA-256 of an entire file.
int checksum_sha256_file(
    const char *filename,
    unsigned char hash[SHA256_HASH_SIZE]
)
{
    if (filename == NULL || hash == NULL)
    {
        printf("[CHECKSUM ERROR] Invalid arguments.\n");
        return -1;
    }

    FILE *file = fopen(filename, "rb");

    if (file == NULL)
    {
        printf("[CHECKSUM ERROR] Could not open file.\n");
        return -1;
    }

    int result = checksum_sha256_stream(
        file,
        0,
        0,
        hash
    );

    fclose(file);

    return result;
}


// Calculate SHA-256 of a specific file region.
int checksum_sha256_file_region(
    const char *filename,
    uint64_t offset,
    uint64_t length,
    unsigned char hash[SHA256_HASH_SIZE]
)
{
    if (filename == NULL || hash == NULL || length == 0)
    {
        printf("[CHECKSUM ERROR] Invalid arguments.\n");
        return -1;
    }

    FILE *file = fopen(filename, "rb");

    if (file == NULL)
    {
        printf("[CHECKSUM ERROR] Could not open file.\n");
        return -1;
    }

    // Move to the requested starting position.
    if (offset > INT64_MAX ||
        _fseeki64(file, (__int64)offset, SEEK_SET) != 0)
    {
        printf("[CHECKSUM ERROR] Could not seek to file region.\n");
        fclose(file);
        return -1;
    }

    int result = checksum_sha256_stream(
        file,
        length,
        1,
        hash
    );

    fclose(file);

    return result;
}


// Print SHA-256 as hexadecimal.
void checksum_print(
    const unsigned char hash[SHA256_HASH_SIZE]
)
{
    for (int i = 0; i < SHA256_HASH_SIZE; i++)
    {
        printf("%02X", hash[i]);
    }

    printf("\n");
}