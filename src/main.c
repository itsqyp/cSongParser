#include <stdio.h>
#include <string.h>

#include "parser.h"
#include "decoder.h"

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
            printf("CSongParser --decode <input.sngp> <output.mp3>\n");
            return 1;
        }

        return decoder_extract_payload(argv[2], argv[3]) == 0 ? 0 : 1;
    }

    // Normal audio parsing and encoding.
    if (argc != 2)
    {
        printf("\nUsage:\n");
        printf("CSongParser <audio-file>\n");
        printf("CSongParser --decode <input.sngp> <output-file>\n");
        return 1;
    }

    return parser_process_file(argv[1]);
}