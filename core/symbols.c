#include "memspace.h"
#include <stdlib.h>
#include <string.h>

void ms_symbol_list_free(SymbolList* list) {
    if (!list) return;
    for (int i = 0; i < list->count; i++) {
        if (list->symbols[i].name) free(list->symbols[i].name);
        if (list->symbols[i].file) free(list->symbols[i].file);
        if (list->symbols[i].signature) free(list->symbols[i].signature);
    }
    if (list->symbols) free(list->symbols);
    
    if (list->relationships) {
        for (int i = 0; i < list->rel_count; i++) {
            if (list->relationships[i].from) free(list->relationships[i].from);
            if (list->relationships[i].to) free(list->relationships[i].to);
            if (list->relationships[i].type) free(list->relationships[i].type);
        }
        free(list->relationships);
    }
    
    free(list);
}

static int add_symbol(SymbolList *list, Symbol sym) {
    if (list->count >= list->capacity) {
        int new_cap = list->capacity == 0 ? 16 : list->capacity * 2;
        Symbol *new_syms = realloc(list->symbols, new_cap * sizeof(Symbol));
        if (!new_syms) return -1;
        list->symbols = new_syms;
        list->capacity = new_cap;
    }
    list->symbols[list->count++] = sym;
    return 0;
}

static int add_relationship(SymbolList *list, const char *from, const char *to, const char *type) {
    if (list->rel_count >= list->rel_capacity) {
        int new_cap = list->rel_capacity == 0 ? 16 : list->rel_capacity * 2;
        Relationship *new_rels = realloc(list->relationships, new_cap * sizeof(Relationship));
        if (!new_rels) return -1;
        list->relationships = new_rels;
        list->rel_capacity = new_cap;
    }
    list->relationships[list->rel_count].from = strdup(from);
    list->relationships[list->rel_count].to = strdup(to);
    list->relationships[list->rel_count].type = strdup(type);
    
    if (!list->relationships[list->rel_count].from || 
        !list->relationships[list->rel_count].to || 
        !list->relationships[list->rel_count].type) {
        if (list->relationships[list->rel_count].from) free(list->relationships[list->rel_count].from);
        if (list->relationships[list->rel_count].to) free(list->relationships[list->rel_count].to);
        if (list->relationships[list->rel_count].type) free(list->relationships[list->rel_count].type);
        return -1;
    }
    
    list->rel_count++;
    return 0;
}

static char *extract_node_string(TSNode node, const char *source_code, uint32_t source_len) {
    uint32_t start = ts_node_start_byte(node);
    uint32_t end = ts_node_end_byte(node);
    if (start > source_len) start = source_len;
    if (end > source_len) end = source_len;
    if (start > end) return NULL;
    uint32_t len = end - start;
    char *str = malloc(len + 1);
    if (!str) return NULL;
    strncpy(str, source_code + start, len);
    str[len] = '\0';
    return str;
}

static void walk_tree(TSNode node, const char *file_path, const char *source_code, uint32_t source_len, SymbolList *list, const char *current_caller) {
    if (ts_node_is_null(node)) return;

    const char *type = ts_node_type(node);
    char *new_caller = NULL;
    
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
                Symbol sym = {0};
                sym.name = extract_node_string(name_node, source_code, source_len);
                sym.signature = extract_node_string(signature_node, source_code, source_len);
                sym.kind = SYMBOL_FUNCTION;
                sym.file = strdup(file_path);
                sym.line = ts_node_start_point(node).row + 1;
                
                if (sym.name) {
                    new_caller = strdup(sym.name);
                }
                
                if (!sym.name || !sym.signature || !sym.file || add_symbol(list, sym) != 0) {
                    if (sym.name) free(sym.name);
                    if (sym.signature) free(sym.signature);
                    if (sym.file) free(sym.file);
                }
            }
        }
    }
    
    if (current_caller && (strcmp(type, "call_expression") == 0 || strcmp(type, "call") == 0)) {
        TSNode func_node = ts_node_child_by_field_name(node, "function", strlen("function"));
        if (!ts_node_is_null(func_node)) {
            char *callee = extract_node_string(func_node, source_code, source_len);
            if (callee) {
                add_relationship(list, current_caller, callee, "calls");
                free(callee);
            }
        }
    }

    const char *next_caller = new_caller ? new_caller : current_caller;

    uint32_t child_count = ts_node_child_count(node);
    for (uint32_t i = 0; i < child_count; i++) {
        TSNode child = ts_node_child(node, i);
        walk_tree(child, file_path, source_code, source_len, list, next_caller);
    }
    
    if (new_caller) {
        free(new_caller);
    }
}

SymbolList* ms_extract_symbols(TSTree* tree, TSLanguage *lang, const char* file_path, const char* source_code, uint32_t source_len) {
    (void)lang;
    SymbolList *list = calloc(1, sizeof(SymbolList));
    if (!list) return NULL;
    TSNode root_node = ts_tree_root_node(tree);
    walk_tree(root_node, file_path, source_code, source_len, list, NULL);
    return list;
}
