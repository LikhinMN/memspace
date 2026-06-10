#include "memspace.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

TSLanguage *tree_sitter_c(void);
TSLanguage *tree_sitter_python(void);
TSLanguage *tree_sitter_javascript(void);

static TSLanguage *detect_language(const char *path) {
    const char *ext = strrchr(path, '.');
    if (!ext) return NULL;
    if (strcmp(ext, ".c") == 0 || strcmp(ext, ".h") == 0) return tree_sitter_c();
    if (strcmp(ext, ".py") == 0) return tree_sitter_python();
    if (strcmp(ext, ".js") == 0) return tree_sitter_javascript();
    return NULL;
}

static char *read_file(const char *path, size_t *out_len) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    if (len < 0) { fclose(f); return NULL; }
    fseek(f, 0, SEEK_SET);
    char *buf = malloc((size_t)len + 1);
    if (!buf) {
        fclose(f);
        return NULL;
    }
    size_t read_bytes = fread(buf, 1, (size_t)len, f);
    buf[read_bytes] = '\0';
    if (out_len) *out_len = read_bytes;
    fclose(f);
    return buf;
}

SymbolList* ms_parse_file(const char* path) {
    size_t len = 0;
    char *source_code = read_file(path, &len);
    if (!source_code) return NULL;

    TSLanguage *lang = detect_language(path);
    if (!lang) {
        free(source_code);
        return NULL;
    }

    TSParser *parser = ts_parser_new();
    if (!parser) {
        free(source_code);
        return NULL;
    }
    ts_parser_set_language(parser, lang);

    TSTree *tree = ts_parser_parse_string(parser, NULL, source_code, (uint32_t)len);
    if (!tree) {
        ts_parser_delete(parser);
        free(source_code);
        return NULL;
    }

    SymbolList *list = ms_extract_symbols(tree, lang, path, source_code, (uint32_t)len);

    ts_tree_delete(tree);
    ts_parser_delete(parser);
    free(source_code);

    return list;
}
