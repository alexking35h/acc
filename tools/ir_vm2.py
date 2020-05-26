"""ACC Linear Intermediate Representation (IR) Interpretter

This script implements an interpretter for ACC's Linear IR. Since
the IR preceeds the final output assembly, this is useful for debugging IR
code generation, and comparing optimisation passes.
"""
import json
import re
import sys
from functools import partial
from argparse import ArgumentParser


class AccProgramStop(Exception):
    """Exception raised if the program should stop (main returns)."""


class AccProgramError(Exception):
    """Exception raised if a runtime error occurs."""


class AccOperation:
    """Class representing a single IR operation."""

    def __init__(self, op, *args):
        self.op = op
        self.args = args
    
    def get_operand_value(self, vm, operand):
        """Evaluate r-value operands

        R-Value operand values for most operations can be:
        - '$.*' - local variable offset (from the SP)
        - '@.*' - global variable address
        - '[a-z].*' - register value.
        """
        if operand.startswith(('$','@')):
            return vm.get_address_label(operand)
        elif re.match('[a-z][0-9]*$', operand):
            return vm.get_reg(operand)
        else:
            raise AccProgramError("Unknown operand '%s' for op '%s', operand, self.op")

    @classmethod
    def from_dict(cls, json_array):
        return cls(json_array[0], *json_array[1:])

    def execute(self, vm):
        """Execute this function.

        Returns True if this operation does not affect the PC.
        """
        # print("execute {}, {}".format(self.op, str(self.args)))
        op_function = {
            "add": partial(self._arithmetic, lambda x, y: x + y),
            "sub": partial(self._arithmetic, lambda x, y: x - y),
            "sll": partial(self._arithmetic, lambda x, y: x << y),
            "slr": partial(self._arithmetic, lambda x, y: x >> y),
            "mul": partial(self._arithmetic, lambda x, y: x * y),
            "div": partial(self._arithmetic, lambda x, y: x / y),
            "load": self._load,
            "store": self._store,
            "loadi": self._loadi,
            "cmp": self._cmp,
            "jmp": partial(self._jmp, lambda vm: 0),
            "je": partial(self._jmp, lambda vm: 0 if vm.is_equal else 1),
            "jlt": partial(self._jmp, lambda vm: 0 if vm.is_less_than else 1),
            "jgt": partial(self._jmp, lambda vm: 0 if vm.is_greater_than else 1),
            "call": self._call,
            "return": self._return,
            "prn": self._prn,
            "prni": self._prni,
        }[self.op]
        op_function(vm)

        # The PC should not be incremented after Jump, call, and return operations.
        return self.op not in ["jmp", "je", "jlt", "jgt", "call", "return"]

    def _arithmetic(self, op, vm):
        """Arithmetic operation."""
        left = self.get_operand_value(vm, self.args[1])
        right = self.get_operand_value(vm, self.args[2])
        vm.set_reg(self.args[0], op(left, right))

    def _load(self, vm):
        """Load from memory to register.

        Syntax: load <destination reg>, <base>, <offset>
        """
        dest_reg = self.args[0]
        base = self.get_operand_value(vm, self.args[1])
        offset = self.get_operand_value(vm, self.args[2])

        vm.set_reg(dest_reg, vm.load(base, offset))

    def _store(self, vm):
        """Store register value to memory.

        Syntax: store <source reg>, <base>, <offset>
        """
        value = vm.get_reg(self.args[0])
        base = self.get_operand_value(vm, self.args[1])
        offset = self.get_operand_value(vm, self.args[2])

        vm.store(value, base, offset)

    def _loadi(self, vm):
        """Load immediate value into register."""
        vm.set_reg(self.args[0], self.args[1])

    def _cmp(self, vm):
        """Compare two registers."""
        vm.set_cmp(self.args[0], self.args[1])

    def _jmp(self, branch, vm):
        """Jump to a branch.

        'branch' is a callable, which returns the index of the argument,
        which should be branched to.
        """
        vm.jump(self.args[branch(vm)])

    def _call(self, vm):
        """Call a function."""
        vm.function_call(self.args[0], self.args[1:])

    def _return(self, vm):
        """Return from a function."""
        vm.function_return()

    def _prn(self, vm):
        """Print register value to stdout."""
        print(chr(vm.get_reg(self.args[0])), end="")
    
    def _prni(self, vm):
        """Print int to stdout."""
        print(vm.get_reg(self.args[0]))


class AccBlock:
    """Class representing an IR Basic Block.
    
    A block is a linear sequence of operations without jumps.
    All basic blocks must end end with a jump or return.
    """

    def __init__(self, code):
        self.code = code

    @classmethod
    def from_dict(cls, json_array):
        return cls([AccOperation.from_dict(j) for j in json_array])


class AccFunction:
    """Class representing a function definition."""

    def __init__(self, local_variables, parameters, blocks):
        self.local_variables = local_variables
        self.parameters = parameters
        self.blocks = blocks

    @classmethod
    def from_dict(cls, json_dict):
        return cls(
            json_dict["local_variables"],
            json_dict["parameters"],
            {b: AccBlock.from_dict(c) for b, c in json_dict["blocks"].items()},
        )

    @property
    def stack_size(self):
        return len(self.local_variables)


class AccProgram:
    """Class representing an entire json program."""

    def __init__(self, functions, constants):
        self.functions = functions
        self.constants = constants

    @classmethod
    def from_json(self, json_src):
        def decode_acc_ir(dct):
            if "functions" in dct:
                return AccProgram(
                    {f: AccFunction.from_dict(c) for f, c in dct["functions"].items()},
                    dct['constants']
                )
            else:
                return dct

        return json.loads(json_src, object_hook=decode_acc_ir)


class AccStackFrame:
    """Class representing a stack frame in the VM."""

    def __init__(self, return_address, function, block, registers):
        self.return_address = return_address
        self.function = function
        self.block = block
        self.registers = registers


class AccVM:
    """Class representing the ACC IR VM.

    This class implements the VM state, including registers and memory.
    """

    def __init__(self, program):
        self.call_stack = []
        self.current_function = None
        self.current_block = None
        self.registers = None
        self.pc = None
        self.sp = 1000
        self.memory = {}
        self._cmp = None

        self.globals = {}
        self._load_program(program)

        # Initialize global variables.
        self._load_program(program)

        self.program = program
    
    def _load_program(self, program):
        """Load a program into the VM."""
        const_ptr = 0
        for name, array in program.constants.items():
            self.globals[name] = const_ptr
            for value in array:
                self.memory[const_ptr] = value
                const_ptr += 1

    def run(self, entry_function="main", debug=False):
        """Run the program."""
        self.current_function = self.program.functions[entry_function]
        self.current_block = self.current_function.blocks[entry_function + "_init"]
        self.pc = 0
        self.registers = {}
        self._debug = debug

        self.debug("Starting program")

        try:
            while True:
                # Fetch the next instruction
                next_op = self.current_block.code[self.pc]

                if next_op.execute(self):
                    self.pc += 1

        except AccProgramStop:
            self.debug("Finished program")

    def set_reg(self, reg_name, reg_value):
        """Set a register value."""
        self.registers[reg_name] = reg_value

    def get_reg(self, reg_name):
        """Retrieve a register value."""
        return self.sp if reg_name == "sp" else self.registers[reg_name]

    def load(self, base, offset):
        """Retrieve a value from memory"""
        return self.memory[base + offset]

    def store(self, value, base, offset):
        """Store a value to memory."""
        self.memory[base + offset] = value

    def set_cmp(self, left, right):
        """Compare two registers."""
        self._cmp = (self.get_reg(left), self.get_reg(right))

    @property
    def is_equal(self):
        return self._cmp[0] == self._cmp[1]

    @property
    def is_less_than(self):
        return self._cmp[0] < self._cmp[1]

    @property
    def is_greater_than(self):
        return self._cmp[0] > self._cmp

    def function_call(self, function, arguments):
        """Call a Function."""
        # Push the current state onto the call stack.
        self.debug(f"Calling function '{function}'")
        self.call_stack.append(
            AccStackFrame(
                return_address=self.pc + 1,
                function=self.current_function,
                block=self.current_block,
                registers=self.registers,
            )
        )

        self.current_function = self.program.functions[function]
        self.current_block = self.current_function.blocks[function + "_init"]
        self.pc = 0
        self.sp -= self.current_function.stack_size

        new_registers = {
            p: self.get_reg(a) for p, a in zip(self.current_function.parameters, arguments)
        }
        self.registers = new_registers

    def function_return(self):
        """Return from a function."""
        # Retrieve the return call state from the call stack.
        self.sp += self.current_function.stack_size

        try:
            stack_frame = self.call_stack.pop()

        except IndexError:
            raise AccProgramStop
        
        # Preserve the 'r' / return register, if set.
        return_value = self.registers.get('r', 0)

        self.current_function = stack_frame.function
        self.current_block = stack_frame.block
        self.pc = stack_frame.return_address
        self.registers = stack_frame.registers
        self.registers['r'] = return_value

        self.debug("Returning from function")

    def jump(self, target):
        """Execute a jump."""
        self.debug(f"Jump to block '{target}'")
        self.current_block = self.current_function.blocks[target]
        self.pc = 0

    def get_address_label(self, label):
        """Get the offset for a local variable or global variable."""
        if label.startswith('$'):
            return list(self.current_function.local_variables).index(label[1:])

        elif label.startswith('@'):
            return list(self.program.constants).index(label[1:])
    
    def debug(self, string):
        if(self._debug):
            print(string)


if __name__ == "__main__":
    arg_parser = ArgumentParser()
    arg_parser.add_argument("--file", action="store", required=False)
    arg_parser.add_argument("--verbose", action="store_true")

    args = arg_parser.parse_args()

    # Run the interpretter.
    if args.file:
        source = open(args.file).read()
    else:
        source = sys.stdin.read()
    
    program = AccProgram.from_json(source)
    AccVM(program).run(debug=args.verbose)
