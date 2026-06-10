#ifndef MEMSPACE_H
#define MEMSPACE_H

#include <tree_sitter/api.h>

typedef enum {
    SYMBOL_FUNCTION,
    SYMBOL_CLASS,
    SYMBOL_VARIABLE,
    SYMBOL_IMPORT
} SymbolKind;

typedef struct {
    char *name;
    SymbolKind kind;
    char *file;
    int line;
    char *signature;
} Symbol;

typedef struct {
    Symbol *symbols;
    int count;
    int capacity;
} SymbolList;

typedef struct {
    char *from;
    char *to;
    char *type;
} Relationship;

struct sqlite3;

typedef struct {
    struct sqlite3 *db;
} Index;

/* CLI command handlers */
int cmd_init(int argc, char **argv);
int cmd_index(int argc, char **argv);
int cmd_update(int argc, char **argv);
int cmd_serve(int argc, char **argv);

/* Parser Layer */
SymbolList* ms_parse_file(const char* path);
void ms_symbol_list_free(SymbolList* list);

/* Index Layer */
Index* ms_index_open(const char* db_path);
void ms_index_close(Index* idx);
int ms_index_insert_symbol(Index* idx, Symbol* sym);
int ms_index_insert_relationship(Index* idx, int from_id, int to_id, const char* type);
SymbolList* ms_index_query_symbol(Index* idx, const char* name);
SymbolList* ms_index_query_callers(Index* idx, const char* name);
int ms_index_delete_file_symbols(Index* idx, const char* file);
SymbolList* ms_index_find_feature(Index* idx, const char* keyword);
SymbolList* ms_index_impact_direct(Index* idx, const char* name);
SymbolList* ms_index_impact_transitive(Index* idx, const char* name);

/* Internal usage */
SymbolList* ms_extract_symbols(TSTree* tree, TSLanguage *lang, const char* file_path, const char* source_code);

#endif /* MEMSPACE_H */
