#include <stdio.h>
#include <stdlib.h>

#include "menu.h"

int menu_get_choice(void)
{
    char input[64];
    char *end_pointer;

    printf("\n");
    printf("================================================\n");
    printf("               MAIN MENU\n");
    printf("================================================\n\n");
    printf("================================================\n");
    printf("            Made By Abir Bro\n");
    printf("================================================\n");
    printf("\n");
    
    printf("       1. Send Files\n");
    printf("       2. Receive Files\n");
    printf("       3. Exit\n\n");

    printf("       Select an option: ");

    if (fgets(input, sizeof(input), stdin) == NULL)
    {
        return 3;
    }

    long choice = strtol(input, &end_pointer, 10);

    /* Skip trailing whitespace. */
    while (*end_pointer == ' ' ||
           *end_pointer == '\t' ||
           *end_pointer == '\r' ||
           *end_pointer == '\n')
    {
        end_pointer++;
    }

    /* Validate input. */
    if (end_pointer == input || *end_pointer != '\0')
    {
        printf("\n[MENU] Invalid input. Please enter 1, 2, or 3.\n");
        return -1;
    }

    if (choice < 1 || choice > 3)
    {
        printf("\n[MENU] Invalid selection. Please enter 1, 2, or 3.\n");
        return -1;
    }

    return (int)choice;
}