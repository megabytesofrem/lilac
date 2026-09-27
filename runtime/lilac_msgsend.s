/* ============================================================================
 * lilac_msgsend.s - Trampoline for dynamic message sending in Lilac runtime.
 * ============================================================================
 */

.global lilac_msgsend
.type lilac_msgsend, @function
.align 16

lilac_msgsend:
    /* Registers used:
        %rdi = receiver pointer
        %rsi = selector address
        %rdx, %rcx, %r8, %r9 = arguments (if any) 
    */

    /* Exit instantly if the receiver pointer is null */
    test %rdi, %rdi
    jz .L_return_nil

    /* Load class pointer from LilacHeader (offset -16 bytes) */
    mov -16(%rdi), %r10 /* %r10 = LilacClass pointer */

    /* Load cache mask & cache base array */
    mov 8(%r10), %r11   /* %r11 = cache mask */
    lea 16(%r10), %rax  /* %rax = &cache[0] */

    /* Continue with message send logic here */

    /* Bitwise hash index calculation for cache lookup */
    mov %rsi, %r10 /* %r10 = selector address */
    and %r11, %r10 /* %r10 = index = selector address & cache mask */
    shl $4, %r10   /* %r10 = index * 16 (size of each cache entry) */
    add %rax, %r10 /* %r10 = &cache[index] */

    /* r10 now holds the address of the cache entry for the given selector */

    /* Read the cache entry */
    mov (%rax), %r10  /* %r10 = entry->selector */
    mov 8(%rax), %rax /* %rax = entry->method */
    cmp %rsi, %r10    /* compare selector address with cache entry selector */
    jne .L_path_miss  /* cache miss: dispatch to lilac_msgsend_slow_path */

    /* cache hit: jump to the method, bypassing prologue/epilogue */
    jmp *%rax

.L_path_miss:
    /* Handle cache miss: call lilac_msgsend_slow_path or similar logic */
    jmp lilac_msgsend_slow_path

.L_return_nil:
    xor %rax, %rax  /* return nil (null pointer) */ 
    ret