#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <sqlite3.h>
#include "memspace.h"
#include "cJSON.h"

static const char* get_kind_str(SymbolKind kind) {
    switch (kind) {
        case SYMBOL_FUNCTION: return "FUNCTION";
        case SYMBOL_CLASS: return "CLASS";
        case SYMBOL_VARIABLE: return "VARIABLE";
        case SYMBOL_IMPORT: return "IMPORT";
        default: return "UNKNOWN";
    }
}

static void handle_get_symbol(Index *idx, cJSON *params, cJSON *response) {
    cJSON *name_item = cJSON_GetObjectItem(params, "name");
    if (!cJSON_IsString(name_item)) {
        cJSON_AddStringToObject(response, "error", "missing or invalid name");
        return;
    }
    SymbolList *list = ms_index_query_symbol(idx, name_item->valuestring);
    if (list && list->count > 0) {
        cJSON *obj = cJSON_CreateObject();
        Symbol *sym = &list->symbols[0];
        cJSON_AddStringToObject(obj, "name", sym->name ? sym->name : "");
        cJSON_AddStringToObject(obj, "kind", get_kind_str(sym->kind));
        cJSON_AddStringToObject(obj, "file", sym->file ? sym->file : "");
        cJSON_AddNumberToObject(obj, "line", sym->line);
        if (sym->signature) {
            cJSON_AddStringToObject(obj, "signature", sym->signature);
        }
        cJSON_AddItemToObject(response, "result", obj);
    } else {
        cJSON_AddStringToObject(response, "error", "symbol not found");
    }
    ms_symbol_list_free(list);
}

static void handle_get_callers(Index *idx, cJSON *params, cJSON *response) {
    cJSON *name_item = cJSON_GetObjectItem(params, "name");
    if (!cJSON_IsString(name_item)) {
        cJSON_AddStringToObject(response, "error", "missing or invalid name");
        return;
    }
    SymbolList *list = ms_index_query_callers(idx, name_item->valuestring);
    cJSON *arr = cJSON_CreateArray();
    if (list) {
        for (int i = 0; i < list->count; i++) {
            cJSON *obj = cJSON_CreateObject();
            cJSON_AddStringToObject(obj, "name", list->symbols[i].name ? list->symbols[i].name : "");
            cJSON_AddStringToObject(obj, "file", list->symbols[i].file ? list->symbols[i].file : "");
            cJSON_AddNumberToObject(obj, "line", list->symbols[i].line);
            cJSON_AddItemToArray(arr, obj);
        }
        ms_symbol_list_free(list);
    }
    cJSON_AddItemToObject(response, "result", arr);
}

static void handle_find_feature(Index *idx, cJSON *params, cJSON *response) {
    cJSON *kw_item = cJSON_GetObjectItem(params, "keyword");
    if (!cJSON_IsString(kw_item)) {
        cJSON_AddStringToObject(response, "error", "missing or invalid keyword");
        return;
    }
    SymbolList *list = ms_index_find_feature(idx, kw_item->valuestring);
    cJSON *arr = cJSON_CreateArray();
    if (list) {
        for (int i = 0; i < list->count; i++) {
            cJSON *obj = cJSON_CreateObject();
            cJSON_AddStringToObject(obj, "name", list->symbols[i].name ? list->symbols[i].name : "");
            cJSON_AddStringToObject(obj, "kind", get_kind_str(list->symbols[i].kind));
            cJSON_AddStringToObject(obj, "file", list->symbols[i].file ? list->symbols[i].file : "");
            cJSON_AddNumberToObject(obj, "line", list->symbols[i].line);
            cJSON_AddItemToArray(arr, obj);
        }
        ms_symbol_list_free(list);
    }
    cJSON_AddItemToObject(response, "result", arr);
}

static void handle_impact_of(Index *idx, cJSON *params, cJSON *response) {
    cJSON *name_item = cJSON_GetObjectItem(params, "name");
    if (!cJSON_IsString(name_item)) {
        cJSON_AddStringToObject(response, "error", "missing or invalid name");
        return;
    }
    SymbolList *direct = ms_index_impact_direct(idx, name_item->valuestring);
    SymbolList *trans = ms_index_impact_transitive(idx, name_item->valuestring);

    cJSON *res_obj = cJSON_CreateObject();
    
    cJSON *arr_dir = cJSON_CreateArray();
    if (direct) {
        for (int i = 0; i < direct->count; i++) {
            cJSON *obj = cJSON_CreateObject();
            cJSON_AddStringToObject(obj, "name", direct->symbols[i].name ? direct->symbols[i].name : "");
            cJSON_AddStringToObject(obj, "file", direct->symbols[i].file ? direct->symbols[i].file : "");
            cJSON_AddItemToArray(arr_dir, obj);
        }
        ms_symbol_list_free(direct);
    }
    
    cJSON *arr_trans = cJSON_CreateArray();
    if (trans) {
        for (int i = 0; i < trans->count; i++) {
            cJSON *obj = cJSON_CreateObject();
            cJSON_AddStringToObject(obj, "name", trans->symbols[i].name ? trans->symbols[i].name : "");
            cJSON_AddStringToObject(obj, "file", trans->symbols[i].file ? trans->symbols[i].file : "");
            cJSON_AddItemToArray(arr_trans, obj);
        }
        ms_symbol_list_free(trans);
    }

    cJSON_AddItemToObject(res_obj, "direct", arr_dir);
    cJSON_AddItemToObject(res_obj, "transitive", arr_trans);
    cJSON_AddItemToObject(response, "result", res_obj);
}

int cmd_serve(int argc, char **argv) {
    (void)argc;
    (void)argv;

    signal(SIGPIPE, SIG_IGN);

    struct stat st;
    if (stat(".memspace/index.db", &st) != 0) {
        fprintf(stderr, "error: no index found. run `memspace index` first\n");
        return 1;
    }

    Index *idx = ms_index_open(".memspace/index.db");
    if (!idx) {
        fprintf(stderr, "error: no index found. run `memspace index` first\n");
        return 1;
    }

    fprintf(stderr, "memspace MCP server ready\n");

    char buf[4096];
    while (fgets(buf, sizeof(buf), stdin)) {
        cJSON *req = cJSON_Parse(buf);
        if (!req) {
            cJSON *res = cJSON_CreateObject();
            cJSON_AddStringToObject(res, "error", "invalid json");
            char *out = cJSON_PrintUnformatted(res);
            fprintf(stdout, "%s\n", out);
            fflush(stdout);
            free(out);
            cJSON_Delete(res);
            continue;
        }

        cJSON *res = cJSON_CreateObject();
        cJSON *tool = cJSON_GetObjectItem(req, "tool");
        cJSON *params = cJSON_GetObjectItem(req, "params");
        
        if (!cJSON_IsString(tool)) {
            cJSON_AddStringToObject(res, "error", "missing tool name");
        } else if (!cJSON_IsObject(params)) {
            cJSON_AddStringToObject(res, "error", "missing params");
        } else {
            const char *tname = tool->valuestring;
            if (strcmp(tname, "get_symbol") == 0) {
                handle_get_symbol(idx, params, res);
            } else if (strcmp(tname, "get_callers") == 0) {
                handle_get_callers(idx, params, res);
            } else if (strcmp(tname, "find_feature") == 0) {
                handle_find_feature(idx, params, res);
            } else if (strcmp(tname, "impact_of") == 0) {
                handle_impact_of(idx, params, res);
            } else {
                cJSON_AddStringToObject(res, "error", "unknown tool");
            }
        }
        
        char *out = cJSON_PrintUnformatted(res);
        fprintf(stdout, "%s\n", out);
        fflush(stdout);
        free(out);
        
        cJSON_Delete(req);
        cJSON_Delete(res);
    }

    ms_index_close(idx);
    return 0;
}
