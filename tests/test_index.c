#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "memspace.h"

int main() {
    printf("Running index tests...\n");
    Index *idx = ms_index_open(":memory:");
    assert(idx != NULL);

    Symbol sym1 = {"test_func", SYMBOL_FUNCTION, "test.c", 10, "void test_func()"};
    Symbol sym2 = {"test_var", SYMBOL_VARIABLE, "test.c", 12, "int test_var"};

    int id1 = ms_index_insert_symbol(idx, &sym1);
    assert(id1 > 0);
    int id2 = ms_index_insert_symbol(idx, &sym2);
    assert(id2 > 0);

    SymbolList *res = ms_index_query_symbol(idx, "test_func");
    assert(res != NULL);
    assert(res->count == 1);
    assert(strcmp(res->symbols[0].name, "test_func") == 0);
    assert(res->symbols[0].kind == SYMBOL_FUNCTION);
    assert(strcmp(res->symbols[0].file, "test.c") == 0);
    assert(res->symbols[0].line == 10);
    
    ms_symbol_list_free(res);
    ms_index_close(idx);

    printf("test_index: PASS\n");
    return 0;
}
