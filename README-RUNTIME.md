# lilac-runtime

Lilac targets Mach-O for its planned code generation backend, as Mach-O natively supports sections in the executable dedicated
to selector and method lookup - which would have required disgusting hacks for ELF.

`runtime/mach` implements the Mach-O loader (`lilac-dyld`) as Linux cannot natively read a Mach-O executable format.