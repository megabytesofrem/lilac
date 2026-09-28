/* ====================================================================================
 * lilac_atomic_x86_64.s - Atomic retain/release for Lilac's object model
 * ====================================================================================
 */
.section .text

.equ OBJ_RETAIN_COUNT, -8

.global lilac_retain
.type lilac_retain, @function
.align 16

/* ============================================================================
 * lilac_retain - Atomically retain a reference to a Lilac object
 *
 * Parameters:
 *   %rdi = self (LilacObject*)
 * Returns into %rax:
 *   LilacObject* if the object is still alive, or nil (null pointer) if it's
 *   reference count reached zero.
 * ============================================================================ */
lilac_retain:
    /* %rdi = self (LilacObject*) */
    testq %rdi, %rdi
    jz .L_retain_nil

    /* Lock the memory bus and atomically increment the reference count */
    lock incq OBJ_RETAIN_COUNT(%rdi)  /* atomic increment of the reference count */

    /* %rax = self */
    movq %rdi, %rax  /* return self in %rax */
    ret
.L_retain_nil:
    xorq %rax, %rax  /* return nil (null pointer) */
    ret

/* ============================================================================
 * lilac_release - Atomically release a reference to a Lilac object
 *
 * Parameters:
 *   %rdi = self (LilacObject*)
 * Returns into %rax:
 *   LilacObject* if the object is still alive, or nil (null pointer) if it's
 *   reference count reached zero.
 * ============================================================================ */
.global lilac_release
.type lilac_release, @function
.align 16

lilac_release:
    /* %rdi = self (LilacObject*) */
    testq %rdi, %rdi
    jz .L_release_nil

    /* Lock the memory bus and atomically decrement the reference count */
    lock decq OBJ_RETAIN_COUNT(%rdi)  /* atomic decrement of the reference count */
    jz .L_release_zero

    /* return self in %rax */
    movq %rdi, %rax    /* return self in %rax */
    ret
.L_release_zero:
    /* Reference count reached zero: perform cleanup if necessary */
    subq $16, %rdi     /* %rdi = pointer to &LilacHeader */
    jmp lilac_dealloc  /* jump to the deallocation routine for the object */
.L_release_nil:
    /* return nil (null pointer) */
    xorq %rax, %rax    /* return nil (null pointer) */
    ret