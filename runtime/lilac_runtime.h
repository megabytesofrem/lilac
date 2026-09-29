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

/* clang-format off */

/* Primary Tags (Bits 0, 1, 2) -> Mask: 0x07 */
#define LILAC_TAG_MASK      0x07
#define LILAC_TAG_PTR       0x00  /* 000 */
#define LILAC_TAG_INT       0x01  /* 001 */
#define LILAC_TAG_FLOAT     0x02  /* 010 */
#define LILAC_TAG_SYMBOL    0x03  /* 011 */
#define LILAC_TAG_SPECIAL   0x04  /* 100 */

/* Special Immediate Constants (Tag 0x04 -> bits 0..2 = 100) */
#define LILAC_NIL           ((LilacWord)(0x04)) /* payload 0: 0000 0100 */
#define LILAC_TRUE          ((LilacWord)(0x0C)) /* payload 1: 0000 1100 */
#define LILAC_FALSE         ((LilacWord)(0x14)) /* payload 2: 0001 0100 */

/* Subtag Values for Integers (Bits 3, 4, 5) -> Shifted by 3 */
#define SUBTAG_MASK         0x38
#define SUBTAG_I8           (0x00 << 3)
#define SUBTAG_U8           (0x01 << 3)
#define SUBTAG_I16          (0x02 << 3)
#define SUBTAG_U16          (0x03 << 3)
#define SUBTAG_I32          (0x04 << 3)
#define SUBTAG_U32          (0x05 << 3)
#define SUBTAG_I58          (0x06 << 3)
/* clang-format on */

typedef uintptr_t LilacWord;

typedef struct LilacClass LilacClass;
typedef struct LilacObject LilacObject;
typedef LilacObject* (*IMP)(LilacObject* receiver, const char* sel, ...);

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

typedef struct LilacString {
    LilacObjectHdr hdr;
    size_t length;
    char* data;
} LilacString;

typedef struct LilacCacheEntry {
    const char* sel;
    IMP method;
} LilacCacheEntry;

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
 * ======================================================= */

LilacWord lilac_encode_i8(int8_t value);
LilacWord lilac_encode_i16(int16_t value);
LilacWord lilac_encode_i32(int32_t value);
LilacWord lilac_encode_u8(uint8_t value);
LilacWord lilac_encode_u16(uint16_t value);
LilacWord lilac_encode_u32(uint32_t value);

int8_t lilac_decode_i8(LilacWord value);
int16_t lilac_decode_i16(LilacWord value);
int32_t lilac_decode_i32(LilacWord value);
uint8_t lilac_decode_u8(LilacWord value);
uint16_t lilac_decode_u16(LilacWord value);
uint32_t lilac_decode_u32(LilacWord value);

bool lilac_is_small_int(LilacWord value);
bool lilac_is_large_int(LilacWord value);

/* ======================================================= */

const char* lilac_sel_registername(const char* name);
LilacClass* lilac_get_class(LilacObject* self);

LilacObjectHdr* lilac_get_object_hdr(LilacObject* self);
LilacObject* lilac_alloc(LilacClass* cls, size_t size);
void lilac_dealloc(LilacObject* self);

LilacObject* lilac_retain(LilacObject* self);
LilacObject* lilac_release(LilacObject* self);

/* Assembly trampolines */
extern LilacObject* lilac_msgsend(LilacObject* self, const char* sel, ...);
extern double lilac_msgsend_fpret(LilacObject* self, const char* sel, ...);
extern void lilac_msgsend_stret(void* sret_buf, LilacObject* self,
                                const char* sel, ...);

/* Slow paths */
IMP lilac_msgsend_slow(LilacObject* self, const char* sel, ...);
IMP lilac_msgsend_tagged_slow(LilacObject* self, const char* sel, ...);
IMP lilac_msgsend_fpret_slow(LilacObject* self, const char* sel, ...);
#endif  // LILAC_RUNTIME_H