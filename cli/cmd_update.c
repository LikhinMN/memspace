#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "memspace.h"

int cmd_update(int argc, char **argv) {
    (void)argc;
    (void)argv;

    if (system("git rev-parse --is-inside-work-tree >/dev/null 2>&1") != 0) {
        return cmd_index(0, NULL);
    }

    FILE *pipe = popen("git diff --name-only HEAD 2>/dev/null", "r");
    if (!pipe) {
        return cmd_index(0, NULL);
    }

    Index *idx = ms_index_open(".memspace/index.db");
    if (!idx) {
        fprintf(stderr, "Failed to open index.db. Did you run init?\n");
        pclose(pipe);
        return 1;
    }

    ms_index_begin_transaction(idx);

    int total_symbols = 0;
    int total_files = 0;
    char line[1024];

    while (fgets(line, sizeof(line), pipe) != NULL) {
        line[strcspn(line, "\n")] = '\0';
        if (strlen(line) == 0) continue;

        char *fname = line;
        if (fname[0] == '"') {
            fname++;
            size_t len = strlen(fname);
            if (len > 0 && fname[len - 1] == '"') {
                fname[len - 1] = '\0';
            }
        }
        
        const char *ext = strrchr(fname, '.');
        if (ext && (strcmp(ext, ".c") == 0 || strcmp(ext, ".py") == 0 || strcmp(ext, ".js") == 0)) {
            ms_index_delete_file_symbols(idx, fname);
            
            SymbolList *list = ms_parse_file(fname);
            if (list) {
                for (int i = 0; i < list->count; i++) {
                    if (ms_index_insert_symbol(idx, &list->symbols[i]) < 0) {
                        // Keep going if one fails
                    }
                }
                total_symbols += list->count;
                total_files++;
                ms_symbol_list_free(list);
            }
        }
    }
    
    ms_index_commit_transaction(idx);

    pclose(pipe);
    ms_index_close(idx);

    printf("Updated %d symbols across %d files\n", total_symbols, total_files);
    return 0;
}
