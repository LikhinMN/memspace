#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "memspace.h"

int cmd_init(int argc, char **argv) {
    (void)argc;
    (void)argv;
    
    struct stat st;
    if (stat(".memspace", &st) != -1) {
        fprintf(stderr, "Error: .memspace/ already exists\n");
        return 1;
    }

    if (system("mkdir -p .memspace") != 0) {
        fprintf(stderr, "Error: failed to create .memspace/ directory\n");
        return 1;
    }

    FILE *f = fopen(".memspace/config.json", "w");
    if (!f) {
        perror("fopen");
        return 1;
    }
    
    if (fprintf(f, "{ \"version\": \"0.1.0\", \"languages\": [\"c\",\"python\",\"javascript\"] }\n") < 0) {
        perror("fprintf");
        fclose(f);
        return 1;
    }
    
    if (fclose(f) != 0) {
        perror("fclose");
        return 1;
    }

    printf("Initialized memspace in .memspace/\n");
    return 0;
}
