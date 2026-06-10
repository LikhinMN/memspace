#include <stdio.h>
#include <string.h>
#include "memspace.h"

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: memspace <command>\n");
        printf("Commands: init, index, update, serve\n");
        return 1;
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
        printf("Unknown command: %s\n", argv[1]);
        return 1;
    }
}
