//
//  my_malloc-driver.c
//  Lab1: Malloc
//


#include "my_malloc.h"
#include <stdio.h>

static void print_free_list(const char *tag)
{
    FreeListNode n = free_list_begin();
    FreeListNode prev = (FreeListNode)0;
    int sorted = 1;
    unsigned count = 0;

    printf("%s\n", tag);

    while (n != (FreeListNode)0) {
        if (count > 1000) {
            printf("  ERROR: free list too long (possible cycle)\n");
            break;
        }

        printf("  node[%u].size=%u\n", count, n->size);

        if (prev != (FreeListNode)0 && (char *)prev > (char *)n) {
            sorted = 0;
        }

        prev = n;
        n = n->flink;
        count++;
    }

    printf("  count=%u sorted=%d\n", count, sorted);
}

static unsigned long mod8(void *p)
{
    return ((unsigned long)p) & 7UL;
}

int main(int argc, const char * argv[])
{
    (void)argc;
    (void)argv;

    printf("=== TEST 1: small allocations, alignment, then free & coalesce ===\n");

    void *p[10];
    unsigned i;

    for (i = 0; i < 10; i++) {
        p[i] = my_malloc((uint32_t)(i + 1));
        printf("malloc(%u): align=%lu errno=%d\n",
               (unsigned)(i + 1),
               (unsigned long)mod8(p[i]),
               (int)my_errno);
    }

    /* After these, there should be one remainder chunk still on the free list */
    print_free_list("Free list after 10 mallocs (should have 1 remainder node):");

    for (i = 10; i > 0; i--) {
        my_free(p[i - 1]);
        printf("free(p[%u]): errno=%d\n", (unsigned)(i - 1), (int)my_errno);
    }

    print_free_list("Free list after freeing all 10 (should be sorted by address):");

    coalesce_free_list();
    print_free_list("Free list after coalesce (ideally 1 big node, often 8192):");

    printf("\n=== TEST 2: splitting from a big free block (post-coalesce) ===\n");

    void *q = my_malloc(100);
    printf("malloc(100): align=%lu errno=%d\n", (unsigned long)mod8(q), (int)my_errno);
    print_free_list("Free list after malloc(100) (should show 1 remainder node):");

    my_free(q);
    printf("free(q): errno=%d\n", (int)my_errno);
    print_free_list("Free list after free(q) (should be 2 adjacent nodes, not merged yet):");

    coalesce_free_list();
    print_free_list("Free list after coalesce (should merge back):");

    printf("\n=== TEST 3: invalid frees ===\n");

    my_free((void *)0);
    printf("free(NULL): errno=%d (expect MYBADFREEPTR)\n", (int)my_errno);

    {
        int stack_var = 123;
        my_free(&stack_var);
        printf("free(&stack_var): errno=%d (expect MYBADFREEPTR)\n", (int)my_errno);
    }

    return 0;
}