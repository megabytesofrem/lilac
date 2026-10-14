# lilac-runtime

Lilac does not target the Executable and Linking Format (ELF), as ELF lacks the ability to express dynamic
selector based messaging. Lilac compiles to freestanding Mach-O using the `x86_64-apple-darwin` flag in `clang`, and provides `lilac-dyld` to parse and read Mach-O binaries and libraries on a Linux host.

This is deliberate, as Mach-O allows us to represent selectors and class information within the binary which ELF misses.

`runtime/mach` implements the Mach-O loader (`lilac-dyld`) as Linux cannot natively read a Mach-O executable format.