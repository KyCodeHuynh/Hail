#include <stdlib.h>
#include <stdio.h> 

int main(int argc, char* argv[]) 
{
    // argc is the argument count, the length of argv[]
    // argv is the argument variable array
    // argc is at least 1, as argv[0] is the program name

    // Need at least 'hostname port filename'
    // For formatting multi-line literal strings,
    // see: http://stackoverflow.com/questions/1135841/c-multiline-string-literal
    if (argc < 4) {
        printf("\nUsage: \t%s hostname portnumber filename [OPTIONS]\n\n"
        "Send a message another endpoint using the Hail protocol.\n\n"
        "Options:\n"
        "-l L, --loss L     Simulate message loss with probability L in [0,1]\n"
        "-c C, --corrupt C  Simulate message corruption with probability C in [0,1]\n"
        "-s, --silent     Run silently without activity output to stdout or stderr\n\n", 
        argv[0]);

        return EXIT_FAILURE;
    }
    // Handle special options first, so that we can use them later
    // TODO: Use getopt() to avoid this counting madness
    else if (argc == 7) {
        printf("Options not yet implemented!\n");
    }

    return EXIT_SUCCESS;
}