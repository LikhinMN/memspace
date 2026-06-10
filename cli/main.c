#include <stdio.h>
#include <string.h>
#include "memspace.h"

int main(int argc, char **argv) {
    if (argc < 2 || strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        printf("Usage: memspace <command>\n");
        printf("Commands:\n");
        printf("  init    Initialize memspace in .memspace/\n");
        printf("  index   Index all supported files in the current directory\n");
        printf("          --lang <language>  Filter by language (c, python, javascript)\n");
        printf("  update  Update index for changed files\n");
        printf("  serve   Start a language server (not implemented)\n");
        return 0;
    }

    if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0) {
        printf("memspace 0.1.0\n");
        return 0;
    }

    if (strcmp(argv[1], "init") == 0) {
        return cmd_init(argc - 1, argv + 1);
    } else if (strcmp(argv[1], "index") == 0) {
        return cmd_index(argc - 1, argv + 1);
    } else if (strcmp(argv[1], "update") == 0) {
        return cmd_update(argc - 1, argv + 1);
    } else if (strcmp(argv[1], "serve") == 0) {
        return cmd_serve(argc - 1, argv + 1);
    } else {
        fprintf(stderr, "Unknown subcommand: %s\n", argv[1]);
        return 1;
    }
}
