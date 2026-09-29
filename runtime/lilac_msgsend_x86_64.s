/* ====================================================================================
 * lilac_msgsend_x86_64.s - Trampolines for dynamic message sending in Lilac runtime.
 * ====================================================================================
 */

.equ CLASS_OFFSET, -16
.equ CACHE_MASK_OFFSET, 8
.equ CACHE_BASE_OFFSET, 16
.equ CACHE_METH_OFFSET, 8

.section .text

/* ============================================================================
 * lilac_msgsend - Selector-based message sending trampoline
 *
 * Parameters:
 *   %rdi = self (receiver object)
 *   %rsi = sel (selector address)
 *   %rdx, %rcx, %r8, %r9 = arguments 1..4
 * Returns into %r8 (method return value):
 *   Method jump address is stored in %r8 on a cache hit.
 * ============================================================================ */

.global lilac_msgsend
.type lilac_msgsend, @function
.align 16

lilac_msgsend:
    /* Nil check: exit instantly if the receiver pointer is null */
    testq %rdi, %rdi
    jz .L_msgsend_nil

    /* Tag check: check if receiver (%rdi) is a tagged immediate value */
    testq $0x07, %rdi
    jnz .L_path_tagged

    /* Fast path: load the class pointer from the object header */
    movq CLASS_OFFSET(%rdi), %r10        /* %r10 = LilacClass pointer */

    /* Load cache mask & cache base array (offset +8 and +16 bytes respectively) */
    movq CACHE_MASK_OFFSET(%r10), %r11   /* %r11 = cache mask */
    movq CACHE_BASE_OFFSET(%r10), %rax   /* %rax = &cache[0] */

    /* ========================================================================================
     * Calculate the cache bucket index by masking the selector address against the cache
     * mask, then scale by the size of each cache entry (16 bytes) to get the final cache index.
     * ======================================================================================== */

    movq %rsi, %r9                       /* %r9 = selector address */
    andq %r11, %r9                       /* %r9 = index = selector address & cache mask */
    shlq $4, %r9                         /* %r9 = index * 16 (size of each cache entry) */
    addq %rax, %r9                       /* %r9 = &cache[index] */

    /* ====================================================================================
     * Read the cache entry for the calculated index.
     * 1. Load the selector from the cache entry into %r11.
     * 2. Load the method pointer from the cache entry into %r8.
     * 3. Compare the loaded selector against %rsi to determine a cache hit or miss.
     * ==================================================================================== */

    movq (%r9), %r11                     /* %r11 = entry->selector */
    movq CACHE_METH_OFFSET(%r9), %r8     /* %r8 = entry->method */
   
    cmpq %rsi, %r11                      /* compare selector address with cache entry selector */
    jne .L_path_miss                     /* cache miss: dispatch to lilac_msgsend_slow_path */

    /* cache hit: jump to the method in %r8, bypassing prologue/epilogue */
    jmp *%r8
.L_path_tagged:
    /* Handle tagged immediate values separately in C */
    jmp lilac_msgsend_tagged_slow
.L_path_miss:
    /* Handle cache miss: call lilac_msgsend_slow */
    jmp lilac_msgsend_slow
.L_msgsend_nil:
    xorq %rax, %rax  /* return nil (null pointer) */ 
    xorq %rdx, %rdx  /* clear %rdx as well */
    ret


/* ============================================================================ 
 * lilac_msgsend_fpret - Selector-based message sending trampoline for floating-point 
 * return values
 *
 * Parameters:
 *   %rdi = self (receiver object)
 *   %rsi = selector address
 *   %rdx, %rcx, %r8, %r9 = arguments 1..3
 *   %xmm0..%xmm7 = floating-point arguments 1..8
 * ============================================================================ */

.global lilac_msgsend_fpret
.type lilac_msgsend_fpret, @function
.align 16
lilac_msgsend_fpret:
    /* Nil check: exit instantly if the receiver pointer is null */
    testq %rdi, %rdi
    jz .L_fpret_nil

    /* Tag check: check if receiver (%rdi) is a tagged immediate value */
    testq $0x07, %rdi
    jnz .L_fpret_path_tagged

    /* Fast path: load the class pointer from the object header */
    movq CLASS_OFFSET(%rdi), %r10       /* %r10 = LilacClass pointer */

    /* Load cache mask & cache base array */
    movq CACHE_MASK_OFFSET(%r10), %r11  /* %r11 = cache mask */
    movq CACHE_BASE_OFFSET(%r10), %r8   /* %r8 = &cache[0] */

    /* ========================================================================================
     * Calculate the cache bucket index by masking the selector address against the cache
     * mask, then scale by the size of each cache entry (16 bytes) to get the final cache index.
     * ======================================================================================== */


    /* Calculate cache index: index = (selector & mask) */
    movq %rsi, %r9                      /* %r9 = selector address (%rsi) */
    andq %r11, %r9                      /* %r9 = index = selector address & cache mask */
    shlq $4, %r9                        /* %r9 = index * 16 (size of each cache entry) */
    addq %r8, %r9                       /* %r9 = &cache[index] */

    /* ====================================================================================
     * Read the cache entry for the calculated index.
     * 1. Load the selector from the cache entry into %r11.
     * 2. Load the method pointer from the cache entry into %r8.
     * 3. Compare the loaded selector against %rsi to determine a cache hit or miss.
     * ==================================================================================== */

    movq (%r9), %r11                    /* %r11 = entry->selector */
    movq CACHE_METH_OFFSET(%r9), %r8    /* %r8 = entry->method */

    cmpq %rsi, %r11                     /* compare selector address with cache entry selector */
    jne .L_fpret_miss                   /* jump to cache miss handler if selectors don't match */

    /* cache hit: jump to the method in %r8, bypassing prologue/epilogue */
    jmp *%r8                            /* jump to the cached method pointer */
.L_fpret_path_tagged:
    jmp lilac_msgsend_tagged_fpret_slow
.L_fpret_miss:
    /* Handle cache miss: call lilac_msgsend_fpret_slow */
    jmp lilac_msgsend_fpret_slow
.L_fpret_nil:
    /* clear return registers from nil floating-point returns */
    xorq %rax, %rax                     /* clear integer return register */
    xorps %xmm0, %xmm0                  /* clear floating-point return register */
    fldz                                /* clear floating-point stack */
    ret                                 /* final return */