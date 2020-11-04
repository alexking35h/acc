"""ARM Aarch32 Emulator

This module reads an ARM ELF file, and executes it on a basic Aarch32 VM.
This emulator -only supports static executables, and will not import any other
library dependencies.

This module reads an ARM ELF file, and executes it on a basic Aarch32 VM.
Only static executables are supported - the emulator does not import any other
library dependencies; and files must only include ARM Aarch32 code - no Thumb code).

A mininal set of system calls are implemented based on the Linux ARM 32-bit EABI:
 - exit (0x01)
 - write (0x04)

The emulator is not compatible with binaries that use stdlib start-up code, since
system calls like brk() are not implemented. The only valid file descriptor for
write is stdout (0x01). Compile code for this emulator with -nostdlib.

This module uses the following third-party libraries:
 - unicorn - ARM Emulator
 - capstone - Dissassembler (for debug messages while the code is running)
 - pyelftools - Python module for reading ELF files

E.g.:
$ echo "
int _start() {
    asm volatile (
        "mov r7, #1\n"
        "mov r0, #99\n"
        "swi #0"
    );
}" > test.c
$ gcc -nostdlib test.c -o test_exe
$ python aarch32 -f test_exe
"""
import logging
import math
import argparse

import unicorn
import capstone
import elftools

from elftools.elf.elffile import ELFFile

ADDRESS_STACK_START = 2 * 1024 * 1024
ADDRESS_STACK_SIZE = 64 * 1024
ADDRESS_STACK_END = ADDRESS_STACK_START + ADDRESS_STACK_SIZE

EXIT_ADDRESS = 0

class Aarch32Vm:
    """Virtual Machine for ARM Aarch 32."""

    def _stack_init(self):
        """Initialize the stack."""
        self._emu.mem_map(ADDRESS_STACK_START, ADDRESS_STACK_END)
        self._emu.reg_write(unicorn.arm_const.UC_ARM_REG_SP, ADDRESS_STACK_END)
    
    def _registers_init(self):
        """Initialize the register values."""
        self._emu.mem_map(1 * 1023 * 1024, 4096)
        self._emu.reg_write(unicorn.arm_const.UC_ARM_REG_R14, (1* 1024 * 1024) - 4)

    def __init__(self):
        """Initialize Aarch32Vm."""
        self._emu = unicorn.Uc(unicorn.UC_ARCH_ARM, unicorn.UC_MODE_ARM)

        self._emu.hook_add(unicorn.UC_HOOK_CODE, self._instr_hook)
        self._emu.hook_add(unicorn.UC_HOOK_INTR, self._interrupt_hook)

        self._stack_init()
        self._registers_init()

        self.stdout = ""
        self.exitcode = 0

    def load_elf(self, elf):
        """Load an ARM ELF file into memory."""
        for ind, segment in enumerate(elf.iter_segments()):
            if segment.header['p_type'] not in ['PT_LOAD']:
                logging.info("skipping segment %d: %s", ind, segment.header['p_type'])
                continue

            addr = segment.header['p_vaddr']
            size = segment.header['p_memsz']

            if addr & (4096 - 1):
                new_addr = 4096 * math.floor(addr / 4096)
                size = size + (new_addr - addr)
                logging.error(
                    "Segment %d address is not a multiple of 4096 (4KB), using 0x%x instead",
                    ind,
                    addr
                )
                addr = new_addr

            if size & (4096 - 1):
                size = 4096 * math.ceil(size / 4096) + 4096
                logging.warning(
                    "Segment %d size is not a multiple of 4096 (4KB), mapping %d instead",
                    ind,
                    size
                )
            
            logging.info("Map segment %u 0x%x - 0x%x (0x%x bytes)", ind, addr, addr + size, size)
            
            self._emu.mem_map(addr, size)
            self._emu.mem_write(addr, segment.data())

    def _instr_hook(self, _, address, size, *args):
        """Instruction callback."""
        mem = self._emu.mem_read(address, size)

        # Dissassemble the instruction mnemonic
        md = capstone.Cs(capstone.CS_ARCH_ARM, capstone.CS_MODE_ARM)
        instr = list(md.disasm(mem, address))[0]
        logging.debug("Execute 0x%x: %s %s", address, instr.mnemonic, instr.op_str)

    def _interrupt_hook(self, _, no, *args):
        """Interrupt hook."""
        if no == 2:
            self._swi_hook()

    def _swi_hook(self):
        """System call (svc) callback."""
        swi_index = self._emu.reg_read(unicorn.arm_const.UC_ARM_REG_R7)
        swi_handlers = {
            1: self._swi_hook_exit,
            4: self._swi_hook_write
        }
        logging.info("System call: %d", swi_index)

        handler = swi_handlers.get(swi_index)
        if handler:
            return handler()
        else:
            logging.warning("Unhandled system call: %d", swi_index)

    def _swi_hook_exit(self):
        """exit() system call."""
        self.exitcode = self._emu.reg_read(unicorn.arm_const.UC_ARM_REG_R0)
        logging.info("Exit return code: %d", self.exitcode)
        self._emu.emu_stop()

    def _swi_hook_write(self):
        """write() system call."""
        fd = self._emu.reg_read(unicorn.arm_const.UC_ARM_REG_R0)
        buf = self._emu.reg_read(unicorn.arm_const.UC_ARM_REG_R1)
        sz = self._emu.reg_read(unicorn.arm_const.UC_ARM_REG_R2)
        logging.info("Write fd=%d, buf=%d, sz=%d", fd, buf, sz)
        
        buf_contents = self._emu.mem_read(buf, sz)
        if fd == 1:
            logging.info("stdout: %s", buf_contents.decode())
            self.stdout += buf_contents.decode()

    def run(self, start_addr):
        """Run."""
        self._emu.emu_start(start_addr, 1 * 1024 * 1024 - 4)
        
        if self._emu.reg_read(unicorn.arm_const.UC_ARM_REG_PC) == 1 * 1024 * 1024 - 4:
            logging.warning("Did not receive exit() system call")

def main():
    """main entry point."""
    parser = argparse.ArgumentParser(description="Arm Aarch32 Emulator")
    parser.add_argument('--file', action='store', help='ELF file to load', required=True)
    parser.add_argument('--quiet', action='store_true', help='No debug information (only stdout)')
    parser.add_argument('--verbose', action='store_true', help='Extra debug information.')

    args = parser.parse_args()

    if args.quiet:
        logging.basicConfig(level=logging.ERROR)
    elif args.verbose:
        logging.basicConfig(level=logging.DEBUG)
    else:
        logging.basicConfig(level=logging.INFO)

    with open('a.out', 'rb') as fil:
        elffile = elftools.elf.elffile.ELFFile(fil)

        vm = Aarch32Vm()
        vm.load_elf(elffile)
        vm.run(elffile.header['e_entry'])

        logging.info('stdout: %s', vm.stdout)
        logging.info('exit code: %d', vm.exitcode)

        print(vm.stdout, end="")
        exit(vm.exitcode) 

if __name__ == "__main__":
    main()