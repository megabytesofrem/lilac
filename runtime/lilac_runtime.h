/* ============================================================================
 * lilac_runtime.h - Core runtime definitions for the Lilac language.
 * ============================================================================
 */

#ifndef LILAC_RUNTIME_H
#define LILAC_RUNTIME_H

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdatomic.h>

// Class structure for Lilac objects.
typedef struct LilacClass LilacClass;

typedef struct
{
    LilacClass *cls;
    _Atomic uint64_t ref_count;
} LilacClassHeader;

typedef struct
{
    const void *selector;
    void *method;
} LilacCacheEntry;

// Trampoline function for message sending, defined in lilac_msgsend.s
extern void *lilac_msgsend(void *receiver, const void *selector, ...);

static inline void *lilac_msgsend_slow_path(void *receiver, const void *selector, ...)
{
    // TODO: Implement the slow path for message sending here.
    // This is the fallback for cache misses in lilac_msgsend.s.
    return NULL;
}

static inline void *lilac_alloc(size_t size)
{
    LilacClassHeader *hdr = (LilacClassHeader *)malloc(sizeof(LilacClassHeader) + size);
    if (!hdr)
        return NULL;

    // Set the class pointer to NULL initially.
    hdr->cls = NULL;

    // Initialize the reference count to 1 before returning the allocated memory.
    atomic_init(&hdr->ref_count, 1);
    return (void *)(hdr + 1);
}

static inline void lilac_retain(void *obj)
{
    if (!obj)
        return;

    // Increment the reference count of the object by 1
    LilacClassHeader *hdr = ((LilacClassHeader *)obj) - 1;
    atomic_fetch_add(&hdr->ref_count, 1);
}

static inline void lilac_release(void *obj)
{
    if (!obj)
        return;

    // Decrement the reference count of the object by 1
    LilacClassHeader *hdr = ((LilacClassHeader *)obj) - 1;

    // Free the object if the reference count reaches zero.
    if (atomic_fetch_sub(&hdr->ref_count, 1) == 1)
        free(hdr);
}

#endif // LILAC_RUNTIME_H