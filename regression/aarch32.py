"""Aarch32 Emulator"""

import logging

import unicorn
from unicorn import arm_const
from elftools.elf.elffile import ELFFile

ADDRESS_START = 0
ADDRESS_SIZE = 2 * 1024 * 1024
ADDRESS_STACK = 2 * 1024 * 1024

logging.basicConfig()

class Aarch32Vm:

    def __init__(self):
        self._emu = unicorn.Uc(unicorn.UC_ARCH_ARM, unicorn.UC_MODE_ARM)
        self._emu.mem_map(0, 2 * 1024 * 1024)
        self._emu.reg_write(arm_const.UC_ARM_REG_SP, ADDRESS_STACK)

        self._emu.hook_add(unicorn.UC_HOOK_CODE, self._instr_hook)
        self._emu.hook_add(unicorn.UC_HOOK_INTR, self._interrupt_hook)

        self._logger = logging.getLogger(self.__class__.__name__)
        self._logger.setLevel(logging.INFO)

        self.stdout = ""
        self.exitcode = 0

    def load_img(self, img, address):
        self._logger.info("Loading program image (size %d, location: %d)", len(img), address)
        self._emu.mem_write(address, img)
   
    def load_elf(self, elf):
        text = elf.get_section_by_name('.text')
        addr = text.header['sh_addr']
        self.load_img(text.data(), addr)

    def _instr_hook(self, _, address, size, *args):
        self._logger.debug("Instruction: %d", address)

    def _interrupt_hook(self, _, no, *args):
        self._logger.info("Interrupt: %d", no)
        if no == 2:
            self._swi_hook()

    def _swi_hook(self):
        swi_index = self._emu.reg_read(arm_const.UC_ARM_REG_R7)
        swi_handlers = {
            1: self._swi_hook_exit,
            4: self._swi_hook_write
        }

        handler = swi_handlers.get(swi_index)
        if handler:
            return handler()
        else:
            self._logger.info("Unhandled software interrupt: %d", swi_index)

    def _swi_hook_exit(self):
        self.exitcode = self._emu.reg_read(arm_const.UC_ARM_REG_R0)
        self._logger.info("Exit return code: %d", self.exitcode)
        self._emu.emu_stop()

    def _swi_hook_write(self):
        fd = self._emu.reg_read(arm_const.UC_ARM_REG_R0)
        buf = self._emu.reg_read(arm_const.UC_ARM_REG_R1)
        sz = self._emu.reg_read(arm_const.UC_ARM_REG_R2)
        self._logger.info("Write fd=%d, buf=%d, sz=%d", fd, buf, sz)
        
        buf_contents = self._emu.mem_read(buf, sz)
        if fd == 1:
            self._logger.info("stdout: %s", buf_contents.decode())
            self.stdout += buf_contents.decode()

    def run(self, start_addr):
        self._emu.emu_start(start_addr, ADDRESS_START + ADDRESS_SIZE, count=1000)

with open('a.out', 'rb') as fil:
    elffile = ELFFile(fil)
    vm = Aarch32Vm()
    vm.load_elf(elffile)
    vm.run(elffile.header['e_entry'])

    print(f"stdout => {vm.stdout}")
    print(f"exit code => {vm.exitcode}")
 


    
    

