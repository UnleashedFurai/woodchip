#include <stdio.h>

void print_usage() {
    printf("Usage: woodchip <option(s)> file\n");
    printf("Options:\n");
    printf("  -h          Print this dialog.\n");
    printf("  -t <value>  Number of instructions processed per second.\n");
    printf("      default: 12\n");
    printf("  -w <value>  Integer scaling of the window.\n");
    printf("      default: 16\n");
}
