#pragma once

#include <string_view>

#include "mach.h"

/**
 * Status code for lilac-dyld loader. 
 * 
 * `DYLD_SUCCESS` indicates success, while other values indicate an error.
 */
enum DyldStatus {
    DYLD_SUCCESS = 0,
    DYLD_ERR_NULL_POINTER,
    DYLD_ERR_BAD_HEADER,
    DYLD_ERR_INVALID_MAGIC,
};

DyldStatus dyld_parse_macho_header(const uint8_t* buffer,
                                   size_t size,
                                   mach_header_64_t* out_header,
                                   const load_command_t** out_first_cmd);

DyldStatus dyld_parse_load_command(const load_command_t* load_cmd,
                                   size_t size,
                                   load_command_t* out_load_cmd);

DyldStatus dyld_parse_segment_command(const segment_command_t* segment_cmd,
                                      size_t size,
                                      segment_command_t* out_segment_cmd);

DyldStatus dyld_parse_section(const section_t* section,
                              size_t size,
                              section_t* out_section);
