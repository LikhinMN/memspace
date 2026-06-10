#define _GNU_SOURCE
#include "memspace.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ftw.h>
#include <sys/stat.h>

static int total_symbols = 0;
static int total_files = 0;
static Index *global_idx = NULL;

static int process_file(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
    (void)sb;

    if (typeflag == FTW_D) {
        const char *base = fpath + ftwbuf->base;
        if (strcmp(base, ".git") == 0 || strcmp(base, "node_modules") == 0 || strcmp(base, "vendor") == 0 || strcmp(base, "bin") == 0 || strcmp(base, ".memspace") == 0) {
            return FTW_SKIP_SUBTREE;
        }
    }

    if (typeflag == FTW_F) {
        const char *ext = strrchr(fpath, '.');
        if (ext && (strcmp(ext, ".c") == 0 || strcmp(ext, ".py") == 0 || strcmp(ext, ".js") == 0)) {
            SymbolList *list = ms_parse_file(fpath);
            if (list) {
                for (int i = 0; i < list->count; i++) {
                    ms_index_insert_symbol(global_idx, &list->symbols[i]);
                }
                total_symbols += list->count;
                total_files++;
                ms_symbol_list_free(list);
            }
        }
    }

    return FTW_CONTINUE;
}

int cmd_index(int argc, char **argv) {
    (void)argc;
    (void)argv;

    system("mkdir -p .memspace");
    global_idx = ms_index_open(".memspace/index.db");
    if (!global_idx) {
        fprintf(stderr, "Failed to open index.db\n");
        return 1;
    }

    int flags = FTW_PHYS | FTW_ACTIONRETVAL;
    if (nftw(".", process_file, 20, flags) == -1) {
        perror("nftw");
    }

    ms_index_close(global_idx);
    printf("indexed %d symbols from %d files\n", total_symbols, total_files);

    return 0;
}
