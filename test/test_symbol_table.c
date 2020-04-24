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

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(create_table),
      cmocka_unit_test(get_symbol_missing),
      cmocka_unit_test(put_symbol)
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}