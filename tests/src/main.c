#include "test_manager.h"

#include "strings/string_tests.h"
#include "memory/dynamic_allocator_tests.h"
#include "containers/darray_tests.h"
#include "containers/freelist_tests.h"
#include "containers/hashtable_tests.h"


int main() {
    test_manager_init();

    string_register_tests();
    dynamic_allocator_register_tests();
    darray_register_tests();
    freelist_register_tests();
    hashtable_register_tests();

    test_manager_run_tests();

    return 0;
}