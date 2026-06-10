#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
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

// Add standard JSON-RPC error format
static void send_jsonrpc_error(cJSON *req, int code, const char *msg) {
    cJSON *res = cJSON_CreateObject();
    cJSON_AddStringToObject(res, "jsonrpc", "2.0");
    cJSON *id = cJSON_GetObjectItem(req, "id");
    if (id) {
        cJSON_AddItemToObject(res, "id", cJSON_Duplicate(id, 1));
    }
    
    cJSON *err_obj = cJSON_CreateObject();
    cJSON_AddNumberToObject(err_obj, "code", code);
    cJSON_AddStringToObject(err_obj, "message", msg);
    cJSON_AddItemToObject(res, "error", err_obj);
    
    char *out = cJSON_PrintUnformatted(res);
    fprintf(stdout, "%s\n", out);
    fflush(stdout);
    free(out);
    cJSON_Delete(res);
}

// Support for standard MCP `tools/list`
static void handle_tools_list(cJSON *res) {
    cJSON *result = cJSON_CreateObject();
    cJSON *tools = cJSON_CreateArray();

    // get_symbol
    cJSON *t1 = cJSON_CreateObject();
    cJSON_AddStringToObject(t1, "name", "get_symbol");
    cJSON_AddStringToObject(t1, "description", "Get details of a specific symbol");
    cJSON *inps1 = cJSON_CreateObject();
    cJSON_AddStringToObject(inps1, "type", "object");
    cJSON *props1 = cJSON_CreateObject();
    cJSON *n1 = cJSON_CreateObject();
    cJSON_AddStringToObject(n1, "type", "string");
    cJSON_AddItemToObject(props1, "name", n1);
    cJSON_AddItemToObject(inps1, "properties", props1);
    cJSON *req1 = cJSON_CreateArray();
    cJSON_AddItemToArray(req1, cJSON_CreateString("name"));
    cJSON_AddItemToObject(inps1, "required", req1);
    cJSON_AddItemToObject(t1, "inputSchema", inps1);
    cJSON_AddItemToArray(tools, t1);

    // get_callers
    cJSON *t2 = cJSON_CreateObject();
    cJSON_AddStringToObject(t2, "name", "get_callers");
    cJSON_AddStringToObject(t2, "description", "Get functions calling a symbol");
    cJSON *inps2 = cJSON_CreateObject();
    cJSON_AddStringToObject(inps2, "type", "object");
    cJSON *props2 = cJSON_CreateObject();
    cJSON *n2 = cJSON_CreateObject();
    cJSON_AddStringToObject(n2, "type", "string");
    cJSON_AddItemToObject(props2, "name", n2);
    cJSON_AddItemToObject(inps2, "properties", props2);
    cJSON *req2 = cJSON_CreateArray();
    cJSON_AddItemToArray(req2, cJSON_CreateString("name"));
    cJSON_AddItemToObject(inps2, "required", req2);
    cJSON_AddItemToObject(t2, "inputSchema", inps2);
    cJSON_AddItemToArray(tools, t2);

    // find_feature
    cJSON *t3 = cJSON_CreateObject();
    cJSON_AddStringToObject(t3, "name", "find_feature");
    cJSON_AddStringToObject(t3, "description", "Fuzzy search by keyword");
    cJSON *inps3 = cJSON_CreateObject();
    cJSON_AddStringToObject(inps3, "type", "object");
    cJSON *props3 = cJSON_CreateObject();
    cJSON *n3 = cJSON_CreateObject();
    cJSON_AddStringToObject(n3, "type", "string");
    cJSON_AddItemToObject(props3, "keyword", n3);
    cJSON_AddItemToObject(inps3, "properties", props3);
    cJSON *req3 = cJSON_CreateArray();
    cJSON_AddItemToArray(req3, cJSON_CreateString("keyword"));
    cJSON_AddItemToObject(inps3, "required", req3);
    cJSON_AddItemToObject(t3, "inputSchema", inps3);
    cJSON_AddItemToArray(tools, t3);

    // impact_of
    cJSON *t4 = cJSON_CreateObject();
    cJSON_AddStringToObject(t4, "name", "impact_of");
    cJSON_AddStringToObject(t4, "description", "Analyze impact of changing a symbol");
    cJSON *inps4 = cJSON_CreateObject();
    cJSON_AddStringToObject(inps4, "type", "object");
    cJSON *props4 = cJSON_CreateObject();
    cJSON *n4 = cJSON_CreateObject();
    cJSON_AddStringToObject(n4, "type", "string");
    cJSON_AddItemToObject(props4, "name", n4);
    cJSON_AddItemToObject(inps4, "properties", props4);
    cJSON *req4 = cJSON_CreateArray();
    cJSON_AddItemToArray(req4, cJSON_CreateString("name"));
    cJSON_AddItemToObject(inps4, "required", req4);
    cJSON_AddItemToObject(t4, "inputSchema", inps4);
    cJSON_AddItemToArray(tools, t4);

    cJSON_AddItemToObject(result, "tools", tools);
    cJSON_AddItemToObject(res, "result", result);
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

    char buf[65536]; // Bump to 64KB for large JSON-RPC inputs
    while (fgets(buf, sizeof(buf), stdin)) {
        cJSON *req = cJSON_Parse(buf);
        if (!req) {
            // Keep generic legacy error for non-RPC completely broken payloads
            cJSON *res = cJSON_CreateObject();
            cJSON_AddStringToObject(res, "error", "invalid json");
            char *out = cJSON_PrintUnformatted(res);
            fprintf(stdout, "%s\n", out);
            fflush(stdout);
            free(out);
            cJSON_Delete(res);
            continue;
        }

        cJSON *method = cJSON_GetObjectItem(req, "method");
        cJSON *tool = cJSON_GetObjectItem(req, "tool");
        
        cJSON *res = cJSON_CreateObject();
        
        // STANDARD MCP PROTOCOL HANDLING (JSON-RPC 2.0)
        if (cJSON_IsString(method)) {
            cJSON_AddStringToObject(res, "jsonrpc", "2.0");
            cJSON *id = cJSON_GetObjectItem(req, "id");
            if (id) {
                cJSON_AddItemToObject(res, "id", cJSON_Duplicate(id, 1));
            }

            const char *mname = method->valuestring;
            if (strcmp(mname, "initialize") == 0) {
                cJSON *result = cJSON_CreateObject();
                cJSON_AddStringToObject(result, "protocolVersion", "2024-11-05");
                cJSON_AddItemToObject(result, "capabilities", cJSON_CreateObject());
                cJSON *sinfo = cJSON_CreateObject();
                cJSON_AddStringToObject(sinfo, "name", "memspace");
                cJSON_AddStringToObject(sinfo, "version", "0.1.0");
                cJSON_AddItemToObject(result, "serverInfo", sinfo);
                cJSON_AddItemToObject(res, "result", result);
            } 
            else if (strcmp(mname, "notifications/initialized") == 0) {
                cJSON_Delete(req);
                cJSON_Delete(res);
                continue; // no response needed
            }
            else if (strcmp(mname, "tools/list") == 0) {
                handle_tools_list(res);
            }
            else if (strcmp(mname, "tools/call") == 0) {
                cJSON *params = cJSON_GetObjectItem(req, "params");
                cJSON *tname_item = params ? cJSON_GetObjectItem(params, "name") : NULL;
                cJSON *args = params ? cJSON_GetObjectItem(params, "arguments") : NULL;
                
                if (!cJSON_IsString(tname_item) || !cJSON_IsObject(args)) {
                    send_jsonrpc_error(req, -32602, "Invalid params");
                    cJSON_Delete(req);
                    cJSON_Delete(res);
                    continue;
                }
                
                cJSON *temp_res = cJSON_CreateObject();
                const char *tname = tname_item->valuestring;
                
                if (strcmp(tname, "get_symbol") == 0) handle_get_symbol(idx, args, temp_res);
                else if (strcmp(tname, "get_callers") == 0) handle_get_callers(idx, args, temp_res);
                else if (strcmp(tname, "find_feature") == 0) handle_find_feature(idx, args, temp_res);
                else if (strcmp(tname, "impact_of") == 0) handle_impact_of(idx, args, temp_res);
                else cJSON_AddStringToObject(temp_res, "error", "unknown tool");

                cJSON *error_item = cJSON_GetObjectItem(temp_res, "error");
                if (error_item) {
                    send_jsonrpc_error(req, -32603, error_item->valuestring);
                    cJSON_Delete(temp_res);
                    cJSON_Delete(req);
                    cJSON_Delete(res);
                    continue;
                }
                
                // Wrap in MCP tool content array
                cJSON *result = cJSON_CreateObject();
                cJSON *content = cJSON_CreateArray();
                cJSON *text_item = cJSON_CreateObject();
                cJSON_AddStringToObject(text_item, "type", "text");
                
                cJSON *res_val = cJSON_GetObjectItem(temp_res, "result");
                char *str_val = cJSON_PrintUnformatted(res_val);
                cJSON_AddStringToObject(text_item, "text", str_val);
                free(str_val);
                
                cJSON_AddItemToArray(content, text_item);
                cJSON_AddItemToObject(result, "content", content);
                cJSON_AddItemToObject(res, "result", result);
                
                cJSON_Delete(temp_res);
            }
            else {
                send_jsonrpc_error(req, -32601, "Method not found");
                cJSON_Delete(req);
                cJSON_Delete(res);
                continue;
            }
        } 
        // LEGACY CUSTOM PROTOCOL (Sprint 5 spec)
        else if (cJSON_IsString(tool)) {
            cJSON *params = cJSON_GetObjectItem(req, "params");
            if (!cJSON_IsObject(params)) {
                cJSON_AddStringToObject(res, "error", "missing params");
            } else {
                const char *tname = tool->valuestring;
                if (strcmp(tname, "get_symbol") == 0) handle_get_symbol(idx, params, res);
                else if (strcmp(tname, "get_callers") == 0) handle_get_callers(idx, params, res);
                else if (strcmp(tname, "find_feature") == 0) handle_find_feature(idx, params, res);
                else if (strcmp(tname, "impact_of") == 0) handle_impact_of(idx, params, res);
                else cJSON_AddStringToObject(res, "error", "unknown tool");
            }
        } else {
            // Not a recognized protocol
            cJSON_AddStringToObject(res, "error", "missing tool name");
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
