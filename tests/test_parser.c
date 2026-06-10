#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "memspace.h"

int main() {
    printf("Running parser tests...\n");
    SymbolList *list = ms_parse_file("tests/fixture.c");
    assert(list != NULL);
    assert(list->count > 0);
    
    int found_function = 0;
    for (int i = 0; i < list->count; i++) {
        if (list->symbols[i].kind == SYMBOL_FUNCTION && strcmp(list->symbols[i].name, "foo") == 0) {
            found_function = 1;
            break;
        }
    }
    assert(found_function == 1);
    ms_symbol_list_free(list);
    printf("test_parser: PASS\n");
    return 0;
}
