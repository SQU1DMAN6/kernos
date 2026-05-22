#include "io.h"

// The actual implementations of read_port, write_port and load_idt are in
// assembly (see kernel.asm). This C file exists so the build system can
// compile an object that depends on io.h if needed.
