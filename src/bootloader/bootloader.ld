ENTRY(reset_handler)

MEMORY
{
	isr_vector (r)	: ORIGIN = 0x08000000, LENGTH = 0xC0 /* vector table */
	boot_version(r)	: ORIGIN = 0x080000C0, LENGTH = 0x14 /* bootloader version */
	flash (rx)	: ORIGIN = 0x080000D4, LENGTH = 0x5000 - 0x14 - 0xC0
	ram (xrw)	: ORIGIN = 0x200000C0, LENGTH = 0x2000 - 192 /* 8K - 192B for vector table */
}

_stack_top = ORIGIN(ram) + LENGTH(ram);

SECTIONS
{
	.isr_vector : {
		KEEP(*(.isr_vector))
	} >isr_vector

	.boot_version : {
		KEEP(*(.boot_version))
	} >boot_version

	.text : {
		KEEP(*(.startup));
		*(.text*)
		*(.rodata*)
	} >flash

	. = ALIGN(4);
	_sfdata = .;

	.data : AT (_sfdata) {
		_sdata = ABSOLUTE(.);
		*(.data*)
		. = ALIGN(4);
		_edata = ABSOLUTE(.);
	} >ram

	.bss : {
		. = ALIGN(4);
		_sbss = ABSOLUTE(.);
		*(.bss*)
		. = ALIGN(4);
		_ebss = ABSOLUTE(.);
	}

	/DISCARD/ : {
		libc.a ( * )
		libm.a ( * )
		libgcc.a ( * )
		*(.interp*)
		*(.dynsym)
		*(.dynstr*)
		*(.dynamic*)
		*(.gnu*)
		*(.rel*)
		*(.ARM*)
	}
}
