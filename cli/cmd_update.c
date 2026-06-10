#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "memspace.h"

int cmd_update(int argc, char **argv) {
    (void)argc;
    (void)argv;

    // Check if it's actually a git repo
    if (system("git rev-parse --is-inside-work-tree >/dev/null 2>&1") != 0) {
        // Fallback to full re-index
        return cmd_index(0, NULL);
    }

    FILE *pipe = popen("git diff --name-only HEAD 2>/dev/null", "r");
    if (!pipe) {
        // Not a git repo or no git command, fallback to full re-index
        return cmd_index(0, NULL);
    }

    Index *idx = ms_index_open(".memspace/index.db");
    if (!idx) {
        fprintf(stderr, "Failed to open index.db. Did you run init?\n");
        pclose(pipe);
        return 1;
    }

    int total_symbols = 0;
    int total_files = 0;
    char line[1024];

    while (fgets(line, sizeof(line), pipe) != NULL) {
        line[strcspn(line, "\n")] = '\0';
        
        const char *ext = strrchr(line, '.');
        if (ext && (strcmp(ext, ".c") == 0 || strcmp(ext, ".py") == 0 || strcmp(ext, ".js") == 0)) {
            // Delete old
            ms_index_delete_file_symbols(idx, line);
            
            // Re-parse
            SymbolList *list = ms_parse_file(line);
            if (list) {
                for (int i = 0; i < list->count; i++) {
                    ms_index_insert_symbol(idx, &list->symbols[i]);
                }
                total_symbols += list->count;
                total_files++;
                ms_symbol_list_free(list);
            }
        }
    }
    
    pclose(pipe);
    ms_index_close(idx);

    printf("Updated %d symbols across %d files\n", total_symbols, total_files);
    return 0;
}
