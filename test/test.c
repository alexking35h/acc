#include <check.h>

TCase* scanner_testcase(void);

int main(void) {
  Suite* suite = suite_create("acc");

  suite_add_tcase(suite, scanner_testcase());

  SRunner* runner = srunner_create(suite);
  srunner_run_all(runner, CK_NORMAL);

  srunner_free(runner);
  return 0;
}
