.set noreorder

#include "pspstub.s"

STUB_START "sceGe_driver", 0x40090000, 0x00010005
STUB_FUNC  0xD8633888, sceGeEdramSetSize
STUB_END

