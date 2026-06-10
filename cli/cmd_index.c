
#include "memspace.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ftw.h>
#include <sys/stat.h>
#include <time.h>

static int total_symbols = 0;
static int total_files = 0;
static Index *global_idx = NULL;
static const char *filter_lang = NULL;

static int process_file(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
    (void)sb;

    if (typeflag == FTW_SL) {
        return FTW_CONTINUE; // Explicitly skip symlinks
    }

    if (typeflag == FTW_D) {
        const char *base = fpath + ftwbuf->base;
        if (strcmp(base, ".git") == 0 || strcmp(base, "node_modules") == 0 || strcmp(base, "vendor") == 0 || strcmp(base, "bin") == 0 || strcmp(base, ".memspace") == 0) {
            return FTW_SKIP_SUBTREE;
        }
    }

    if (typeflag == FTW_F) {
        const char *ext = strrchr(fpath, '.');
        if (ext) {
            if (filter_lang) {
                if (strcmp(filter_lang, "c") == 0 && strcmp(ext, ".c") != 0) return FTW_CONTINUE;
                if (strcmp(filter_lang, "python") == 0 && strcmp(ext, ".py") != 0) return FTW_CONTINUE;
                if (strcmp(filter_lang, "javascript") == 0 && strcmp(ext, ".js") != 0) return FTW_CONTINUE;
            }

            if (strcmp(ext, ".c") == 0 || strcmp(ext, ".py") == 0 || strcmp(ext, ".js") == 0) {
                SymbolList *list = ms_parse_file(fpath);
                if (list) {
                    for (int i = 0; i < list->count; i++) {
                        int id = ms_index_insert_symbol(global_idx, &list->symbols[i]);
                        if (id >= 0 && list->relationships) {
                            for (int j = 0; j < list->rel_count; j++) {
                                if (strcmp(list->relationships[j].from, list->symbols[i].name) == 0) {
                                    ms_index_add_unresolved_relationship(global_idx, id, list->relationships[j].to, list->relationships[j].type);
                                }
                            }
                        }
                    }
                    total_symbols += list->count;
                    total_files++;
                    ms_symbol_list_free(list);
                }
            }
        }
    }

    return FTW_CONTINUE;
}

int cmd_index(int argc, char **argv) {
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "--lang") == 0 && i + 1 < argc) {
            filter_lang = argv[i + 1];
        }
    }

    struct stat st;
    if (stat(".memspace", &st) == -1) {
        fprintf(stderr, "Error: .memspace/ not found. Run init first.\n");
        return 1;
    }

    global_idx = ms_index_open(".memspace/index.db");
    if (!global_idx) {
        fprintf(stderr, "Failed to open index.db\n");
        return 1;
    }
    
    total_symbols = 0;
    total_files = 0;

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    ms_index_begin_transaction(global_idx);

    int flags = FTW_PHYS | FTW_ACTIONRETVAL;
    if (nftw(".", process_file, 20, flags) == -1) {
        perror("nftw");
    }

    ms_index_resolve_relationships(global_idx);
    ms_index_commit_transaction(global_idx);

    clock_gettime(CLOCK_MONOTONIC, &end);
    double ms = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1000000.0;

    ms_index_close(global_idx);
    
    printf("Indexed %d symbols in %.1fms\n", total_symbols, ms);

    return 0;
}
