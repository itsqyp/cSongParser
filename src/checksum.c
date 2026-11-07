#include <stdio.h>
#include <windows.h>
#include <bcrypt.h>

#include "checksum.h"

#define BUFFER_SIZE 4096

int checksum_sha256_file(
    const char *filename,
    unsigned char hash[SHA256_HASH_SIZE])
{
    FILE *file = NULL;

    BCRYPT_ALG_HANDLE algorithm = NULL;
    BCRYPT_HASH_HANDLE hash_handle = NULL;

    NTSTATUS status;

    unsigned char buffer[BUFFER_SIZE];

    DWORD object_size = 0;
    DWORD hash_size = 0;
    DWORD result_size = 0;

    unsigned char *hash_object = NULL;

    // Open the input file.
    file = fopen(filename, "rb");

    if (file == NULL)
    {
        printf("[CHECKSUM ERROR] Could not open file.\n");
        return -1;
    }

    // Open the SHA-256 algorithm provider.
    status = BCryptOpenAlgorithmProvider(
        &algorithm,
        BCRYPT_SHA256_ALGORITHM,
        NULL,
        0
    );

    if (status != 0)
    {
        printf("[CHECKSUM ERROR] Could not initialize SHA-256.\n");
        fclose(file);
        return -1;
    }

    // Get the required hash object size.
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

    // Get the SHA-256 output size.
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

    // Allocate memory required by Windows CNG.
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

    // Create the hash handle.
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

    // Read the file and update the hash incrementally.
    size_t bytes_read;

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0)
    {
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
    }

    // Check for file reading errors.
    if (ferror(file))
    {
        printf("[CHECKSUM ERROR] File reading failed.\n");
        goto cleanup;
    }

    // Finalize SHA-256 calculation.
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

    // Successful calculation.
    BCryptDestroyHash(hash_handle);
    BCryptCloseAlgorithmProvider(algorithm, 0);

    HeapFree(GetProcessHeap(), 0, hash_object);
    fclose(file);

    return 0;

cleanup:

    if (hash_handle != NULL)
        BCryptDestroyHash(hash_handle);

    if (algorithm != NULL)
        BCryptCloseAlgorithmProvider(algorithm, 0);

    if (hash_object != NULL)
        HeapFree(GetProcessHeap(), 0, hash_object);

    if (file != NULL)
        fclose(file);

    return -1;
}


// Print a SHA-256 hash as hexadecimal.
void checksum_print(
    const unsigned char hash[SHA256_HASH_SIZE])
{
    for (int i = 0; i < SHA256_HASH_SIZE; i++)
    {
        printf("%02X", hash[i]);
    }

    printf("\n");
}