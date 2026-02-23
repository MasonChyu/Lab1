//
//  my_malloc.c
//  Lab1: Malloc
//

#include "my_malloc.h"
#include <unistd.h>   // sbrk

MyErrorNo my_errno = MYNOERROR;

/* Only allowed global (besides my_errno) */
static FreeListNode g_free_list_head = (FreeListNode)0;


static const uint32_t MY_MALLOC_MAGIC = 0xC0FFEE42u;



static uint32_t align_up_8(uint32_t n)
{
    return (n + 7u) & ~7u;
}

static uint32_t max_u32(uint32_t a, uint32_t b)
{
    return (a > b) ? a : b;
}

static uint32_t min_chunk_size(void)
{
    
    return max_u32(16u, (uint32_t)sizeof(struct freelistnode));
}

static uint32_t required_chunk_size(uint32_t payload_size)
{
    /* total = header(8) + payload, rounded up to multiple of 8, at least min chunk */
    uint32_t total = align_up_8(payload_size + 8u);
    uint32_t minsz = min_chunk_size();
    if (total < minsz) {
        total = minsz;
    }
    return total;
}

static void header_write(void *chunk_start, uint32_t chunk_size)
{
    uint32_t *hdr = (uint32_t *)chunk_start;
    hdr[0] = chunk_size;
    hdr[1] = MY_MALLOC_MAGIC;
}

static uint32_t header_read_size(const void *chunk_start)
{
    const uint32_t *hdr = (const uint32_t *)chunk_start;
    return hdr[0];
}

static uint32_t header_read_magic(const void *chunk_start)
{
    const uint32_t *hdr = (const uint32_t *)chunk_start;
    return hdr[1];
}

static void *chunk_to_payload(void *chunk_start)
{
    return (void *)((char *)chunk_start + 8);
}

static void *payload_to_chunk(void *payload_ptr)
{
    return (void *)((char *)payload_ptr - 8);
}

/* Insert node into free list in ascending address order */
static void free_list_insert_sorted(FreeListNode node)
{
    FreeListNode prev;
    FreeListNode curr;

    if (node == (FreeListNode)0) {
        return;
    }

    if (g_free_list_head == (FreeListNode)0 || (char *)node < (char *)g_free_list_head) {
        node->flink = g_free_list_head;
        g_free_list_head = node;
        return;
    }

    prev = g_free_list_head;
    curr = g_free_list_head->flink;

    while (curr != (FreeListNode)0 && (char *)curr < (char *)node) {
        prev = curr;
        curr = curr->flink;
    }

    prev->flink = node;
    node->flink = curr;
}

/* Remove first-fit node with size >= needed; return it or 0 */
static FreeListNode free_list_remove_first_fit(uint32_t needed)
{
    FreeListNode prev = (FreeListNode)0;
    FreeListNode curr = g_free_list_head;

    while (curr != (FreeListNode)0) {
        if (curr->size >= needed) {
            if (prev == (FreeListNode)0) {
                g_free_list_head = curr->flink;
            } else {
                prev->flink = curr->flink;
            }
            return curr;
        }
        prev = curr;
        curr = curr->flink;
    }
    return (FreeListNode)0;
}


void *my_malloc(uint32_t size)
{
    uint32_t needed = required_chunk_size(size);
    FreeListNode chunk = (FreeListNode)0;
    uint32_t chunk_size = 0;

    
    chunk = free_list_remove_first_fit(needed);

    if (chunk != (FreeListNode)0) {
        uint32_t avail = chunk->size;
        uint32_t rem = avail - needed;

        if (rem >= min_chunk_size()) {
            FreeListNode rnode = (FreeListNode)((char *)chunk + needed);
            rnode->size = rem;
            rnode->flink = (FreeListNode)0;
            free_list_insert_sorted(rnode);
            chunk_size = needed;
        } else {
            /* don't split */
            chunk_size = avail;
        }
    } else {
        /* 2) Grow heap via sbrk() */
        uint32_t incr = (needed > 8192u) ? needed : 8192u;
        incr = align_up_8(incr);

        void *region = sbrk((long)incr);
        if (region == (void *)-1) {
            my_errno = MYENOMEM;
            return (void *)0;
        }

        chunk = (FreeListNode)region;

        /* Split new region if worthwhile */
        {
            uint32_t rem = incr - needed;
            if (rem >= min_chunk_size()) {
                FreeListNode rnode = (FreeListNode)((char *)region + needed);
                rnode->size = rem;
                rnode->flink = (FreeListNode)0;
                free_list_insert_sorted(rnode);
                chunk_size = needed;
            } else {
                chunk_size = incr;
            }
        }
    }

    /* 3) Header + return  */
    header_write((void *)chunk, chunk_size);
    my_errno = MYNOERROR;
    return chunk_to_payload((void *)chunk);
}

void my_free(void *ptr)
{
    void *chunk_start;
    uint32_t magic;
    uint32_t csize;
    FreeListNode node;

    if (ptr == (void *)0) {
        my_errno = MYBADFREEPTR;
        return;
    }

    chunk_start = payload_to_chunk(ptr);
    magic = header_read_magic(chunk_start);

    if (magic != MY_MALLOC_MAGIC) {
        my_errno = MYBADFREEPTR;
        return;
    }

    csize = header_read_size(chunk_start);

    /* Overlay freelist node on freed chunk */
    node = (FreeListNode)chunk_start;
    node->size = csize;
    node->flink = (FreeListNode)0;

    /* Insert sorted by address */
    free_list_insert_sorted(node);

    /* No coalescing*/
    my_errno = MYNOERROR;
}

FreeListNode free_list_begin(void)
{
    return g_free_list_head;
}

void coalesce_free_list(void)
{
    FreeListNode curr = g_free_list_head;

    while (curr != (FreeListNode)0 && curr->flink != (FreeListNode)0) {
        FreeListNode next = curr->flink;
        char *curr_end = (char *)curr + curr->size;

        if (curr_end == (char *)next) {
            curr->size += next->size;
            curr->flink = next->flink;
        } else {
            curr = curr->flink;
        }
    }
}