#pragma once

#include <cstddef>
#include <cstdint>

typedef int cpu_type_t;
typedef int cpu_subtype_t;

/* CPU types */
#define CPU_ARCH_ABI64 0x01000000

#define CPU_TYPE_X86 (cpu_type_t)7
#define CPU_TYPE_X86_64 (cpu_type_t)(CPU_TYPE_X86 | CPU_ARCH_ABI64)
#define CPU_TYPE_ARM (cpu_type_t)12
#define CPU_TYPE_ARM64 (cpu_type_t)(CPU_TYPE_ARM | CPU_ARCH_ABI64)

/* CPU subtypes */
#define CPU_SUBTYPE_MASK 0xff000000        /* mask for feature flags */
#define CPU_SUBTYPE_LIB64 0x80000000       /* for 64 bit libraries */
#define CPU_SUBTYPE_INTENT_MASK 0x00ffffff /* mask for intent flags */

#define CPU_SUBTYPE_ANY (cpu_subtype_t) - 1      /* any CPU subtype */
#define CPU_SUBTYPE_MULTIPLE (cpu_subtype_t) - 2 /* multiple CPU subtypes */

/* Mask to extract the intent portion of a CPU subtype */
#define CPU_SUBTYPE(subtype) \
    ((cpu_subtype_t)((subtype) & CPU_SUBTYPE_INTENT_MASK))

#define CPU_ARCH_IS_64(cputype) (((cputype) & CPU_ARCH_ABI64) != 0)

/*
 * x86 subtypes
 */
#define CPU_SUBTYPE_X86_ALL (cpu_subtype_t)3
#define CPU_SUBTYPE_X86_64_ALL (cpu_subtype_t)3
#define CPU_SUBTYPE_X86_64_H (cpu_subtype_t)8 /* Haswell */

/*
 * ARM/ARM64 subtypes
 */
#define CPU_SUBTYPE_ARM_ALL (cpu_subtype_t)0
#define CPU_SUBTYPE_ARM64_ALL (cpu_subtype_t)0
#define CPU_SUBTYPE_ARM64_V8 (cpu_subtype_t)1
#define CPU_SUBTYPE_ARM64E (cpu_subtype_t)2 /* Pointer Authentication/PAC */