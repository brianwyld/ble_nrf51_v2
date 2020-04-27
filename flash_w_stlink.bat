@echo on

set arg2=%2
set major_minor=%arg2:~6,2%%arg2:~4,2%%arg2:~2,2%%arg2:~0,2%
set OPENOCD=c:/Program Files (x86)/OpenOCD
openocd -f "./scripts/interface/stlink.cfg" -f "./scripts/target/nrf51.cfg" -c "init; halt; nrf51 mass_erase; program ./hexs/softdevice.hex verify; program ./hexs/%1 verify; flash fillw 0x000338A0 0x30303030 1; flash fillw 0x000338A4 0x%major_minor% 1; flash fillw 0x000338A8 0xFF0064EC 1; flash fillw 0x00033CA0 0x00000002 1; reset;shutdown"

::
::Command: flash erase_address [pad] [unlock] address length
::
::    Erase sectors starting at address for length bytes. Unless pad is specified, address must begin a flash sector, and address + length - 1 must end a sector. Specifying pad erases extra data at the beginning and/or end of the specified region, as needed to erase only full sectors. The flash bank to use is inferred from the address, and the specified length must stay within that bank. As a special case, when length is zero and address is the start of the bank, the whole flash is erased. If unlock is specified, then the flash is unprotected before erase starts. 
::
::Command: flash filld address double-word length
::Command: flash fillw address word length
::Command: flash fillh address halfword length
::Command: flash fillb address byte length
::
::   Fills flash memory with the specified double-word (64 bits), word (32 bits), halfword (16 bits), or byte (8-bit) pattern, starting at address and continuing for length units (word/halfword/byte). No erasure is done before writing; when needed, that must be done before issuing this command. Writes are done in blocks of up to 1024 bytes, and each write is verified by reading back the data and comparing it to what was written. The flash bank to use is inferred from the address of each block, and the specified length must stay within that bank. 

exit /b