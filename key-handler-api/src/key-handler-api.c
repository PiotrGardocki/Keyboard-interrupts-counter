#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void print_help(void)
{
    printf("Available commands:\n\n");
    printf("key-handler-api --help                      \t - show this help\n");
    printf("key-handler-api, key-handler-api --get-count\t - show interrupts counter\n");
    printf("key-handler-api --get-reset-date            \t - show last counter reset\n");
    printf("key-handler-api --reset                     \t - reset interrupts counter\n");
}

void get_interrupts_count(void)
{
    system("tail -1 /dev/key-handler");
}

void get_reset_date(void)
{
    system("head -1 /dev/key-handler");
}

void reset_counter(void)
{
    system("echo reset > /dev/key-handler");
}

int main(int argc, char ** argv)
{
    if (argc > 2)
    {
        printf("Unsupported operation\n");
        print_help();
    }
    else if (argc == 1)
    {
        get_interrupts_count();
    }
    else if (argc == 2)
    {
        if (strcmp(argv[1], "--help") == 0)
            print_help();
        else if (strcmp(argv[1], "--get-count") == 0)
            get_interrupts_count();
        else if (strcmp(argv[1], "--get-reset-date") == 0)
            get_reset_date();
        else if (strcmp(argv[1], "--reset") == 0)
            reset_counter();
        else
        {
            printf("Unsupported operation\n");
            print_help();
        }
    }

    return 0;
}
