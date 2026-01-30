	.set noreorder

#include "pspstub.s"

	STUB_START "kcall",0x40090000,0x00010005
	STUB_FUNC  0x5AC35241,kcall
	STUB_END
