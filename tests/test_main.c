#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

/* Test cases */
static void test_basic(void) {
    assert(1 == 1);
    printf("Basic test passed!\n");
}

static void test_another(void) {
    assert(2 + 2 == 4);
    printf("Another test passed!\n");
}

/* Test runner */
int main(void) {
    printf("Running tests...\n");
    
    test_basic();
    test_another();
    
    printf("All tests passed successfully!\n");
    return EXIT_SUCCESS;
}
