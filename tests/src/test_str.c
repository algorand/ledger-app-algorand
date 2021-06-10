#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>

#include <cmocka.h>

#include "algo_ui.h"
#include "str.h"

void test_adjust_decimals(void **state) {
    (void) state;

    char buf[128];
    assert_true(adjustDecimals("123", 3, buf, 7, 4));
    assert_string_equal(buf, "0.0123");

    assert_false(adjustDecimals("123", 3, buf, 6, 4));
}

void test_u64str(void **state) {
    (void) state;

    assert_string_equal(u64str(0xffffffffffffffff), "18446744073709551615");
    assert_string_equal(u64str(0), "0");
}

void test_amount_to_str(void **state) {
    (void) state;

    assert_string_equal(amount_to_str(123456, 4), "12.3456");
    assert_string_equal(amount_to_str(0xffffffffffffffff, 4), "1844674407370955.1615");
    assert_string_equal(amount_to_str(0xffffffffffffffff, 20), "0.18446744073709551615");
}

void test_ui_text_put_str(void **state) {
    (void) state;

    ui_text_put_str("abcd");
    assert_string_equal(text, "abcd");

    char buf[129] = { 0 };
    memset(buf, 'a', sizeof(buf)-1);
    ui_text_put_str(buf);
    assert_string_equal(text + 120, "aa[...]");

    ui_text_put_str("ab\x90z");
    assert_string_equal(text, "ab?z");
}

int main() {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_adjust_decimals),
        cmocka_unit_test(test_u64str),
        cmocka_unit_test(test_amount_to_str),
        cmocka_unit_test(test_ui_text_put_str),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
