#define _POSIX_C_SOURCE 200809L
#include "memspace.h"
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *SCHEMA = 
    "CREATE TABLE IF NOT EXISTS symbols ("
    "  id INTEGER PRIMARY KEY,"
    "  name TEXT,"
    "  kind TEXT,"
    "  file TEXT,"
    "  line INTEGER,"
    "  signature TEXT"
    ");"
    "CREATE TABLE IF NOT EXISTS relationships ("
    "  id INTEGER PRIMARY KEY,"
    "  from_id INTEGER,"
    "  to_id INTEGER,"
    "  type TEXT"
    ");"
    "CREATE TABLE IF NOT EXISTS meta ("
    "  key TEXT,"
    "  value TEXT"
    ");";

Index* ms_index_open(const char* db_path) {
    sqlite3 *db;
    int rc = sqlite3_open(db_path, &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return NULL;
    }

    char *err_msg = NULL;
    rc = sqlite3_exec(db, SCHEMA, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return NULL;
    }

    Index *idx = malloc(sizeof(Index));
    idx->db = db;
    return idx;
}

void ms_index_close(Index* idx) {
    if (idx) {
        if (idx->db) sqlite3_close(idx->db);
        free(idx);
    }
}

static const char* kind_to_string(SymbolKind kind) {
    switch (kind) {
        case SYMBOL_FUNCTION: return "FUNCTION";
        case SYMBOL_CLASS: return "CLASS";
        case SYMBOL_VARIABLE: return "VARIABLE";
        case SYMBOL_IMPORT: return "IMPORT";
        default: return "UNKNOWN";
    }
}

static SymbolKind string_to_kind(const char* str) {
    if (strcmp(str, "FUNCTION") == 0) return SYMBOL_FUNCTION;
    if (strcmp(str, "CLASS") == 0) return SYMBOL_CLASS;
    if (strcmp(str, "VARIABLE") == 0) return SYMBOL_VARIABLE;
    if (strcmp(str, "IMPORT") == 0) return SYMBOL_IMPORT;
    return SYMBOL_FUNCTION;
}

int ms_index_insert_symbol(Index* idx, Symbol* sym) {
    if (!idx || !idx->db || !sym) return -1;
    
    const char *sql = "INSERT INTO symbols (name, kind, file, line, signature) VALUES (?, ?, ?, ?, ?);";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(idx->db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return -1;

    sqlite3_bind_text(stmt, 1, sym->name, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, kind_to_string(sym->kind), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, sym->file, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, sym->line);
    sqlite3_bind_text(stmt, 5, sym->signature, -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc == SQLITE_DONE) {
        return (int)sqlite3_last_insert_rowid(idx->db);
    }
    return -1;
}

int ms_index_insert_relationship(Index* idx, int from_id, int to_id, const char* type) {
    if (!idx || !idx->db) return -1;

    const char *sql = "INSERT INTO relationships (from_id, to_id, type) VALUES (?, ?, ?);";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(idx->db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return -1;

    sqlite3_bind_int(stmt, 1, from_id);
    sqlite3_bind_int(stmt, 2, to_id);
    sqlite3_bind_text(stmt, 3, type, -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc == SQLITE_DONE) {
        return (int)sqlite3_last_insert_rowid(idx->db);
    }
    return -1;
}

static SymbolList* build_symbol_list_from_stmt(sqlite3_stmt *stmt) {
    SymbolList *list = calloc(1, sizeof(SymbolList));
    list->capacity = 16;
    list->symbols = malloc(list->capacity * sizeof(Symbol));

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (list->count >= list->capacity) {
            list->capacity *= 2;
            list->symbols = realloc(list->symbols, list->capacity * sizeof(Symbol));
        }

        Symbol *sym = &list->symbols[list->count++];
        const char *name = (const char*)sqlite3_column_text(stmt, 1);
        const char *kind = (const char*)sqlite3_column_text(stmt, 2);
        const char *file = (const char*)sqlite3_column_text(stmt, 3);
        int line = sqlite3_column_int(stmt, 4);
        const char *signature = (const char*)sqlite3_column_text(stmt, 5);

        sym->name = name ? strdup(name) : NULL;
        sym->kind = kind ? string_to_kind(kind) : SYMBOL_FUNCTION;
        sym->file = file ? strdup(file) : NULL;
        sym->line = line;
        sym->signature = signature ? strdup(signature) : NULL;
    }
    return list;
}

SymbolList* ms_index_query_symbol(Index* idx, const char* name) {
    if (!idx || !idx->db || !name) return NULL;

    const char *sql = "SELECT id, name, kind, file, line, signature FROM symbols WHERE name = ?;";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(idx->db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return NULL;

    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
    
    SymbolList *list = build_symbol_list_from_stmt(stmt);
    sqlite3_finalize(stmt);
    return list;
}

SymbolList* ms_index_query_callers(Index* idx, const char* name) {
    if (!idx || !idx->db || !name) return NULL;

    const char *sql = 
        "SELECT s.id, s.name, s.kind, s.file, s.line, s.signature "
        "FROM symbols s "
        "JOIN relationships r ON s.id = r.from_id "
        "JOIN symbols callee ON r.to_id = callee.id "
        "WHERE callee.name = ? AND r.type = 'calls';";

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(idx->db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return NULL;

    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);

    SymbolList *list = build_symbol_list_from_stmt(stmt);
    sqlite3_finalize(stmt);
    return list;
}

int ms_index_delete_file_symbols(Index* idx, const char* file) {
    if (!idx || !idx->db || !file) return -1;
    const char *sql = "DELETE FROM symbols WHERE file = ?;";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(idx->db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return -1;
    sqlite3_bind_text(stmt, 1, file, -1, SQLITE_TRANSIENT);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return (rc == SQLITE_DONE) ? 0 : -1;
}
