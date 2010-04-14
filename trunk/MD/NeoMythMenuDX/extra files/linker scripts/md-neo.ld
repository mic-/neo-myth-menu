OUTPUT_ARCH(m68k)
SEARCH_DIR(.)

/*
 * Setup the memory map of the SEGA Genesis.
 * stack grows down from high memory.
 *
 * The memory map look like this:
 * +--------------------+ <- low memory
 * | .text              |
 * |        _etext      |
 * |        ctor list   | the ctor and dtor lists are for
 * |        dtor list   | C++ support
 * +--------------------+
 * | .data              | initialized data goes here
 * |        _data       |
 * |        _edata      |
 * +--------------------+
 * | .bss               |
 * |        _bstart     | start of bss, cleared by crt0
 * |        _bend,_end  | start of heap, used by sbrk()
 * +--------------------+
 * .                    .
 * .                    .
 * .                    .
 * |        __stack     | top of stack
 * +--------------------+
 */

MEMORY
{
    rom : ORIGIN = 0x00000000, LENGTH = 0x00100000
   bram : ORIGIN = 0x00200000, LENGTH = 0x00100000
   fram : ORIGIN = 0x00300000, LENGTH = 0x00100000
    ram : ORIGIN = 0x00FF0000, LENGTH = 0x00010000
}

/*
 * allocate the stack to be at the top of memory, since the stack
 * grows down
 */

PROVIDE (__stack = 0x01000000);

SECTIONS
{
  .text 0x00000000 :
  {
    *(.text)
    . = ALIGN(0x4);
     __CTOR_LIST__ = .;
    LONG((__CTOR_END__ - __CTOR_LIST__) / 4 - 2)
    *(.ctors)
    LONG(0)
    __CTOR_END__ = .;
    __DTOR_LIST__ = .;
    LONG((__DTOR_END__ - __DTOR_LIST__) / 4 - 2)
    *(.dtors)
     LONG(0)
    __DTOR_END__ = .;
    *(.rodata*)
    *(.lit)
    _etext = .;
  } > rom
  _stext = SIZEOF (.text);

  .data 0x00FF0000 :
  AT ( LOADADDR (.text) + SIZEOF (.text) )
  {
    _data = . ;
    *(.shdata)
    *(.data)
    *(.eh_fram)
    *(.eh_frame)
    _edata = . ;
  } > ram
  _sdata = SIZEOF (.data);

  .fixed 0x00300000 :
  AT ( LOADADDR (.text) + SIZEOF (.text) + SIZEOF (.data) )
  {
    _fixed = . ;
    *(.fixed)
    _efixed = . ;
    _end = .; PROVIDE (end = .);
  } > fram
  _sfixed = SIZEOF (.fixed);

  .bss 0x00FF0000 + SIZEOF (.data) :
  {
    _bstart = . ;
    *(.shbss)
    *(.bss)
    *(COMMON)
    _bend =  . ;
    __bend = _bend;
    . = ALIGN(0x4);
  } > ram

}
