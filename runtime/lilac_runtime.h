/* ============================================================================
 * lilac_runtime.h - Core runtime definitions for the Lilac language.
 * ============================================================================
 */

#ifndef LILAC_RUNTIME_H
#define LILAC_RUNTIME_H

#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

typedef LilacObject* (*IMP)(LilacObject* receiver, const char* sel, ...);

typedef struct LilacCacheEntry {
    const char* sel;
    IMP method;
} LilacCacheEntry;

/**
 * Object header structure for Lilac's runtime
 *
 * NOTE: Lilac allocates objects with `LilacObjectHdr` before the object data.
 *
 * Memory layout:
 * =================================
 * [ -16 bytes: LilacClass* isa ]
 * [ -8 bytes:  uintptr_t retain_count ]
 * [  0 bytes:  LilacObject data, self pointer ]
 */
typedef struct LilacObjectHdr {
    LilacClass* isa;
    uintptr_t retain_count;
} LilacObjectHdr;

typedef struct LilacObject {
} LilacObject;

/**
 * Class metadata structure for Lilac's runtime
 *
 * - superclass: pointer to the superclass of this class.
 * - cache_mask: mask used for indexing into the method cache.
 * - cache_entries: pointer to the array of method cache entries.
 */
typedef struct LilacClass {
    LilacClass* superclass;
    uintptr_t cache_mask;           /* CACHE_MASK_OFFSET */
    LilacCacheEntry* cache_entries; /* CACHE_BASE_OFFSET */

    /* Additional class metadata can be added here */
} LilacClass;

/* =======================================================
 * C API prototypes
 * =======================================================
 */

const char* lilac_sel_registername(const char* name);
LilacObjectHdr* lilac_get_object_hdr(LilacObject* self);
LilacObject* lilac_object_alloc(LilacClass* cls, size_t size);
LilacObject* lilac_retain(LilacObject* self);
LilacObject* lilac_release(LilacObject* self);
static LilacClass* lilac_get_class(LilacObject* self);

/* Assembly trampolines */
extern LilacObject* lilac_msgsend(LilacObject* self, const char* sel, ...);
extern double lilac_msgsend_fpret(LilacObject* self, const char* sel, ...);
extern void lilac_msgsend_stret(void* sret_buf, LilacObject* self,
                                const char* sel, ...);
IMP lilac_msgsend_slow_path(LilacObject* self, const char* sel, ...);

#endif  // LILAC_RUNTIME_H