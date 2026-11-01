#include <stdio.h>

#include "parser.h"

int main(int argc, char *argv[])
{
    printf("====================================\n");
    printf("         C SONG PARSER v1.0         \n");
    printf("====================================\n\n");

    printf("Binary Audio Processing System\n");
    printf("Status: Initializing...\n");

    // Check whether a filename was provided

    if (argc < 2)
    {
        printf("\nUsage: CSongParser <filename>\n");
        return 1;
    }

    // Process the supplied file

    return parser_process_file(argv[1]);
}