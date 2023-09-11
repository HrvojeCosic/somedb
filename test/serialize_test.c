#include "../include/utils/serialize.h"

#include <check.h>
#include <stdlib.h>

START_TEST(test_encode_decode_uint32) {
    uint32_t original_data = 0x12345678;
    uint8_t buf[4];

    encode_uint32(original_data, buf);
    uint32_t decoded_data = decode_uint32(buf);

    ck_assert_uint_eq(decoded_data, original_data);
}

END_TEST

START_TEST(test_encode_decode_uint16) {
    uint16_t original_data = 0x1234;
    uint8_t buf[2];

    encode_uint16(original_data, buf);
    uint16_t decoded_data = decode_uint16(buf);

    ck_assert_uint_eq(decoded_data, original_data);
}

END_TEST

START_TEST(test_encode_decode_int32) {
    uint8_t buf[4];

    int32_t original_data_neg = -12345678;
    encode_int32(original_data_neg, buf);
    int32_t decoded_data_neg = decode_int32(buf);
    ck_assert_int_eq(decoded_data_neg, original_data_neg);

    int32_t original_data_pos = 12345678;
    encode_int32(original_data_pos, buf);
    int32_t decoded_data_pos = decode_int32(buf);
    ck_assert_int_eq(decoded_data_pos, original_data_pos);
}

END_TEST

START_TEST(test_encode_decode_int16) {
    uint8_t buf[2];

    int16_t original_data_neg = -1234;
    encode_int16(original_data_neg, buf);
    int16_t decoded_data_neg = decode_int16(buf);
    ck_assert_int_eq(decoded_data_neg, original_data_neg);

    int32_t original_data_pos = 1234;
    encode_int16(original_data_pos, buf);
    int32_t decoded_data_pos = decode_int16(buf);
    ck_assert_int_eq(decoded_data_pos, original_data_pos);
}

END_TEST

START_TEST(test_encode_decode_double) {
    double original_data = 1234.5678;
    uint8_t buf[sizeof(double)];

    encode_double(original_data, buf);
    double decoded_data = decode_double(buf);

    ck_assert_double_eq_tol(decoded_data, original_data, 1e-9);
}

END_TEST

START_TEST(test_encode_decode_bool) {
    uint8_t buf[1];

    bool original_data_t = true;
    encode_bool(original_data_t, buf);
    bool decoded_data_t = decode_bool(buf);
    ck_assert_int_eq(decoded_data_t, original_data_t);

    bool original_data_f = false;
    encode_bool(original_data_f, buf);
    bool decoded_data_f = decode_bool(buf);
    ck_assert_int_eq(decoded_data_f, original_data_f);
}

END_TEST

Suite *serialize_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("Serialize");

    tc_core = tcase_create("Core");
    tcase_add_test(tc_core, test_encode_decode_uint32);
    tcase_add_test(tc_core, test_encode_decode_uint16);
    tcase_add_test(tc_core, test_encode_decode_int32);
    tcase_add_test(tc_core, test_encode_decode_int16);
    tcase_add_test(tc_core, test_encode_decode_double);
    tcase_add_test(tc_core, test_encode_decode_bool);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void) {
    Suite *s;
    SRunner *sr;

    s = serialize_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
