#include "lilac_runtime.h"

#include <stdatomic.h>
#include <stdlib.h>

const char* lilac_sel_registername(const char* name) {
    // TODO: Implement selector registration logic here.
    return name;
}

LilacClass* lilac_get_class(LilacObject* self) {
    if (!self) return NULL;

    LilacObjectHdr* hdr = lilac_get_object_hdr(self);

    /* Return the class pointer (isa) from the object header. */
    return hdr->isa;
}

LilacObjectHdr* lilac_get_object_hdr(LilacObject* self) {
    if (!self) return NULL;

    return (LilacObjectHdr*)self - 1;
}

LilacObject* lilac_alloc(LilacClass* cls, size_t size) {
    size_t total_size = sizeof(LilacObjectHdr) + size;
    LilacObjectHdr* hdr = (LilacObjectHdr*)calloc(1, total_size);

    if (!hdr) return NULL;

    hdr->isa = cls;
    hdr->retain_count = 1;

    /* Return a pointer to the object data, which is located immediately after
     * the header. */
    return (LilacObject*)(hdr + 1);
}

void lilac_dealloc(LilacObject* self) {
    if (!self) return;

    /* Deallocate the object by freeing its object header. */
    LilacObjectHdr* hdr = lilac_get_object_hdr(self);
    free(hdr);

    return;
}

LilacObject* lilac_retain(LilacObject* self) {
    if (!self) return NULL;

    LilacObjectHdr* hdr = lilac_get_object_hdr(self);

    /* Atomically fetch and increment the retain count, using acquire/release
     * memory order (relaxed memory order). */
    atomic_fetch_add_explicit((_Atomic uintptr_t*)&hdr->retain_count, 1,
                              memory_order_relaxed);
    return self;
}

LilacObject* lilac_release(LilacObject* self) {
    if (!self) return NULL;

    LilacObjectHdr* hdr = lilac_get_object_hdr(self);

    /* Atomically fetch and decrement the retain count, using relaxed memory
     order. We return the previous value. */
    uintptr_t prev_retain_count = atomic_fetch_sub_explicit(
        (_Atomic uintptr_t*)&hdr->retain_count, 1, memory_order_acq_rel);

    if (prev_retain_count == 1) {
        /* Deallocate the object since its retain count reached zero. */
        /* TODO: lilac_msgsend(self, lilac_sel_registername("dealloc")); */
        free(hdr);
        return NULL;
    }

    return self;
}