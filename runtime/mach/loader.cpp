#include "loader.h"

#include <cstdio>
#include <cstring>

/**
 * Parse a Mach-O header, returning a DyldStatus code indicating success or failure.
 */
DyldStatus dyld_parse_macho_header(const uint8_t* buffer,
                                   size_t size,
                                   mach_header_64_t* out_header,
                                   const load_command_t** out_first_cmd) {
    if (!buffer || !out_header || !out_first_cmd) {
        return DYLD_ERR_NULL_POINTER;
    }

    // Ensure we have enough bytes to read the header
    if (size < sizeof(mach_header_64_t)) {
        return DYLD_ERR_BAD_HEADER;
    }

    // Copy header from the buffer into the output structure
    std::memcpy(out_header, buffer, sizeof(mach_header_64_t));

    // Check MAGIC
    if (out_header->magic != MH_MAGIC_64) {
        std::printf(
            "[lilac-dyld]: Invalid magic number, expected 0x%x, got 0x%x\n",
            MH_MAGIC_64, out_header->magic);
        return DYLD_ERR_INVALID_MAGIC;
    }

    // Check architecture: we only support x86_64
    if (out_header->cputype != CPU_TYPE_X86_64) {
        std::printf(
            "[lilac-dyld]: Unsupported CPU type, expected 0x%x, got 0x%x\n",
            CPU_TYPE_X86_64, out_header->cputype);
        return DYLD_ERR_BAD_HEADER;
    }

    // Parse the first load command
    *out_first_cmd = reinterpret_cast<const load_command_t*>(
        buffer + sizeof(mach_header_64_t));

    return DYLD_SUCCESS;
}

DyldStatus dyld_parse_load_command(const load_command_t* load_cmd,
                                   size_t size,
                                   load_command_t* out_load_cmd) {
    if (!load_cmd || !out_load_cmd) {
        return DYLD_ERR_NULL_POINTER;
    }

    if (size < sizeof(load_command_t)) {
        return DYLD_ERR_BAD_HEADER;
    }

    // Copy the load command from the input to the output structure
    std::memcpy(out_load_cmd, load_cmd, sizeof(load_command_t));

    return DYLD_SUCCESS;
}

DyldStatus dyld_parse_segment_command(const segment_command_t* segment_cmd,
                                      size_t size,
                                      segment_command_t* out_segment_cmd) {
    if (!segment_cmd || !out_segment_cmd) {
        return DYLD_ERR_NULL_POINTER;
    }

    if (size < sizeof(segment_command_t)) {
        return DYLD_ERR_BAD_HEADER;
    }

    // Copy the segment command from the input to the output structure
    std::memcpy(out_segment_cmd, segment_cmd, sizeof(segment_command_t));

    return DYLD_SUCCESS;
}

DyldStatus dyld_parse_section(const section_t* section,
                              size_t size,
                              section_t* out_section) {
    if (!section || !out_section) {
        return DYLD_ERR_NULL_POINTER;
    }

    if (size < sizeof(section_t)) {
        return DYLD_ERR_BAD_HEADER;
    }

    // Copy the section from the input to the output structure
    std::memcpy(out_section, section, sizeof(section_t));

    return DYLD_SUCCESS;
}
