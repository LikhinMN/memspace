#define _POSIX_C_SOURCE 200809L
#include "memspace.h"
#include <stdlib.h>
#include <string.h>

void ms_symbol_list_free(SymbolList* list) {
    if (!list) return;
    for (int i = 0; i < list->count; i++) {
        free(list->symbols[i].name);
        free(list->symbols[i].file);
        if (list->symbols[i].signature) free(list->symbols[i].signature);
    }
    free(list->symbols);
    free(list);
}

static void add_symbol(SymbolList *list, Symbol sym) {
    if (list->count >= list->capacity) {
        list->capacity = list->capacity == 0 ? 16 : list->capacity * 2;
        list->symbols = realloc(list->symbols, list->capacity * sizeof(Symbol));
    }
    list->symbols[list->count++] = sym;
}

static char *extract_node_string(TSNode node, const char *source_code) {
    uint32_t start = ts_node_start_byte(node);
    uint32_t end = ts_node_end_byte(node);
    uint32_t len = end - start;
    char *str = malloc(len + 1);
    strncpy(str, source_code + start, len);
    str[len] = '\0';
    return str;
}

static void walk_tree(TSNode node, const char *file_path, const char *source_code, SymbolList *list) {
    if (ts_node_is_null(node)) return;

    const char *type = ts_node_type(node);
    
    if (strcmp(type, "function_definition") == 0 || strcmp(type, "function_declaration") == 0) {
        TSNode name_node = ts_node_child_by_field_name(node, "name", strlen("name"));
        TSNode signature_node = node;
        
        if (ts_node_is_null(name_node)) {
            TSNode declarator = ts_node_child_by_field_name(node, "declarator", strlen("declarator"));
            if (!ts_node_is_null(declarator)) {
                signature_node = declarator;
                name_node = ts_node_child_by_field_name(declarator, "declarator", strlen("declarator"));
                if (ts_node_is_null(name_node) || strcmp(ts_node_type(name_node), "identifier") != 0) {
                    name_node = declarator;
                }
            }
        }
        
        if (!ts_node_is_null(name_node)) {
            while (strcmp(ts_node_type(name_node), "identifier") != 0 && ts_node_child_count(name_node) > 0) {
                TSNode next = ts_node_child_by_field_name(name_node, "declarator", strlen("declarator"));
                if (ts_node_is_null(next)) {
                    next = ts_node_child(name_node, 0);
                }
                name_node = next;
            }

            if (strcmp(ts_node_type(name_node), "identifier") == 0) {
                Symbol sym;
                sym.name = extract_node_string(name_node, source_code);
                sym.kind = SYMBOL_FUNCTION;
                sym.file = strdup(file_path);
                sym.line = ts_node_start_point(node).row + 1;
                sym.signature = extract_node_string(signature_node, source_code);
                add_symbol(list, sym);
            }
        }
    }

    uint32_t child_count = ts_node_child_count(node);
    for (uint32_t i = 0; i < child_count; i++) {
        TSNode child = ts_node_child(node, i);
        walk_tree(child, file_path, source_code, list);
    }
}

SymbolList* ms_extract_symbols(TSTree* tree, TSLanguage *lang, const char* file_path, const char* source_code) {
    (void)lang;
    SymbolList *list = calloc(1, sizeof(SymbolList));
    TSNode root_node = ts_tree_root_node(tree);
    walk_tree(root_node, file_path, source_code, list);
    return list;
}
