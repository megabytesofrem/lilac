#pragma once

#include <cstddef>
#include <cstdint>

#include "machine.h"

#define MH_MAGIC_32 0xfeedface
#define MH_MAGIC_64 0xfeedfacf
#define MH_CIGAM_32 0xcefaedfe /* reversed byte order */
#define MH_CIGAM_64 0xcffaedfe /* reversed byte order */

// Sources:
// https://github.com/aidansteele/osx-abi-macho-file-format-reference
// https://github.com/apple-oss-distributions/xnu/blob/main/EXTERNAL_HEADERS/mach-o/loader.h
// https://github.com/apple-oss-distributions/xnu/blob/main/osfmk/mach/machine.h

/*
 * 32-bit Mach-O header structure.
 *
 * The mach header appears at the start of the object file (if the object file is 32 bit)
 */
struct mach_header {
    uint32_t magic;           /* mach magic number */
    cpu_type_t cputype;       /* CPU type specifier*/
    cpu_subtype_t cpusubtype; /* CPU subtype specifier */
    uint32_t filetype;        /* type of file */
    uint32_t ncmds;           /* number of load commands */
    uint32_t sizeofcmds;      /* size of all load commands */
    uint32_t flags;           /* flags */
};

/*
 * 64-bit Mach-O header structure.
 *
 * The mach header appears at the start of the object file (if the object file is 64 bit)
 */
struct mach_header_64 {
    uint32_t magic;           /* mach magic number */
    cpu_type_t cputype;       /* CPU type specifier*/
    cpu_subtype_t cpusubtype; /* CPU subtype specifier */
    uint32_t filetype;        /* type of file */
    uint32_t ncmds;           /* number of load commands */
    uint32_t sizeofcmds;      /* size of all load commands */
    uint32_t flags;           /* flags */
    uint32_t reserved;        /* reserved */
};

/**
 * Constants for the `filetype` field in the Mach-O header.
 */
#define MH_OBJECT 0x1     /* relocatable object file */
#define MH_EXECUTE 0x2    /* demand paged executable file */
#define MH_FVMLIB 0x3     /* fixed VM shared library file */
#define MH_CORE 0x4       /* core file */
#define MH_PRELOAD 0x5    /* preloaded executable file */
#define MH_DYLIB 0x6      /* dynamically bound shared library */
#define MH_DYLINKER 0x7   /* dynamic link editor */
#define MH_BUNDLE 0x8     /* dynamically bound bundle file */
#define MH_DYLIB_STUB 0x9 /* shared library stub for static linking */

/* FIXME: Not sure if the rest is needed */

/**
 * Constants for the `flags` field in the Mach-O header.
 */
#define MH_NOUNDEFS 0x1    /* no undefined references */
#define MH_INCRLINK 0x2    /* the object file is the output of an incremental link */
#define MH_DYLDLINK 0x4    /* the object file is the input for the dynamic linker */
#define MH_BINDATLOAD 0x8  /* the object file has its dynamic undefined references bound at load time */
#define MH_PREBOUND 0x10   /* the object file is prebound */
#define MH_SPLIT_SEGS 0x20 /* the object file has its segments split */
#define MH_LAZY_INIT 0x40  /* the shared library initialization is lazy (obsolete) */
#define MH_TWOLEVEL 0x80   /* the object file uses two-level namespace bindings */

struct load_command {
    uint32_t cmd;     /* type of load command */
    uint32_t cmdsize; /* total size of command in bytes */
};

/**
 * Constants for the load command types in the Mach-O header.
 */
#define LC_REQ_DYLD 0x80000000 /* this load command is required by the dynamic linker */

#define LC_SEGMENT_64 0x19 /* 64-bit segment of this file to be mapped */
#define LC_SYMTAB 0x2      /* link-edit stab symbol table info */
#define LC_LOAD_DYLIB 0xc  /* load a dynamically linked shared library */
#define LC_DYLINKER 0xe    /* dynamic link editor command */
#define LC_DYLD_INFO 0x22  /* compressed dyld information */
#define LC_DYLD_INFO_ONLY (0x22 | LC_REQ_DYLD)
#define LC_MAIN (0x28 | LC_REQ_DYLD)

/**
 * 64-bit segment command structure.
 */
struct segment_command_64 {
    uint32_t cmd;
    uint32_t cmdsize;  /* total size of command in bytes */
    char segname[16];  /* segment name */
    uint64_t vmaddr;   /* memory address of this segment */
    uint64_t vmsize;   /* memory size of this segment */
    uint64_t fileoff;  /* file offset of this segment */
    uint64_t filesize; /* length of this segment in the file */
    uint32_t maxprot;  /* maximum VM protection */
    uint32_t initprot; /* initial VM protection */
    uint32_t nsects;   /* number of sections in this segment */
    uint32_t flags;    /* flags */
};

/**
 * 64-bit section structure within a segment.
 */
struct section_64 {
    char sectname[16];  /* name of this section */
    char segname[16];   /* name of the segment this section belongs to */
    uint64_t addr;      /* memory address of this section */
    uint64_t size;      /* size in bytes of this section */
    uint32_t offset;    /* file offset of this section */
    uint32_t align;     /* section alignment (power of 2) */
    uint32_t reloff;    /* file offset of relocation entries */
    uint32_t nreloc;    /* number of relocation entries */
    uint32_t flags;     /* flags (section type and attributes) */
    uint32_t reserved1; /* reserved (for offset or index) */
    uint32_t reserved2; /* reserved (for count or sizeof) */
    uint32_t reserved3; /* reserved (for future use) */
};

/**
 * Mach-O entry point command structure.
 */
struct entry_point_command {
    uint32_t cmd;       /* LC_MAIN only */
    uint32_t cmdsize;   /* total size of command in bytes */
    uint64_t entryoff;  /* file offset of main() */
    uint64_t stacksize; /* initial stack size */
};

/* Typedefs for Mach-O structures */
typedef struct mach_header mach_header_t;
typedef struct mach_header_64 mach_header_64_t;
typedef struct load_command load_command_t;
typedef struct segment_command_64 segment_command_64_t;
typedef struct section_64 section_64_t;
typedef struct entry_point_command entry_point_command_t;