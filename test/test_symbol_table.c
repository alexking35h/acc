#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <cmocka.h>

#include "symbol.h"

static void create_table(void** state) {
    symbol_table_create(NULL);
}

static void get_symbol_missing(void** state) {
    SymbolTable* tab = symbol_table_create(NULL);
    assert_true(symbol_table_get(tab, "abcdef", false) == NULL);
}

static void put_symbol(void** state) {
    SymbolTable* tab = symbol_table_create(NULL);

    symbol_table_put(tab, "testing", (CType*)0x1234);
    symbol_table_put(tab, "testing2", (CType*)0x2345);

    Symbol* sym = symbol_table_get(tab, "testing", false);
    assert_true(sym->type == (CType*)0x1234);
    sym = symbol_table_get(tab, "testing2", false);
    assert_true(sym->type == (CType*)0x2345);
}

static void get_symbol_nested(void** state) {
    SymbolTable* tab = symbol_table_create(NULL);
    symbol_table_put(tab, "s1", (CType*)0x4567);
    symbol_table_put(tab, "s2", (CType*)0x9876);

    SymbolTable* nested = symbol_table_create(tab);
    symbol_table_put(nested, "s1", (CType*)0xabcd);

    // Should find 's1' in the nested scope, which
    // shadows 's1' in the outer scope.
    Symbol* s1 = symbol_table_get(nested, "s1", true);
    assert_true(s1->type == (CType*)0xabcd);

    // Should find 's2' in the outer scope.
    Symbol* s2 = symbol_table_get(nested, "s2", true);
    assert_true(s2->type == (CType*)0x9876);

    // Should not find 's2' in the nested scope.
    assert_true(symbol_table_get(nested, "s2", false) == NULL);
}

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(create_table),
      cmocka_unit_test(get_symbol_missing),
      cmocka_unit_test(put_symbol),
      cmocka_unit_test(get_symbol_nested),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}