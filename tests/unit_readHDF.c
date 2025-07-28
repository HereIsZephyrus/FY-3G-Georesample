#include "test_suites.h"
#include "interface.h"
#include <string.h>
static const char* TEST_FILE = "/mnt/repo/hxlc/FY-3G-Georesample/tests/FY3G_PMR--_ORBA_L1_20250519_1924_5000M_V1.HDF";

herr_t attr_iterator(hid_t location_id, const char *attr_name, const H5A_info_t *ainfo, void *opdata) {
    printf("属性名: '%s'\n", attr_name);

    // 显示属性名的每个字符（用于调试特殊字符）
    printf("  字符编码: ");
    for (int i = 0; attr_name[i] != '\0'; i++) 
        printf("%d ", (unsigned char)attr_name[i]);

    printf("\n");
    return 0;
}

void test_readHDF5(void) {
    //hid_t file_id = H5Fopen(TEST_FILE, H5F_ACC_RDONLY, H5P_DEFAULT);
    //H5Aiterate2(file_id, H5_INDEX_NAME, H5_ITER_INC, NULL, attr_iterator, NULL);
    //H5Fclose(file_id);
    TEST_ASSERT_EQUAL(true, ReadHDF5(TEST_FILE));
}