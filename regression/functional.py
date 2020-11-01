import abc
import subprocess
import json
from os import path

TEMPLATE_EXPRESSION = """
int main() {{
    if({expression}) return 0;
    return 1;
}}
"""

TEMPLATE_BODY = """
int main() {{
    {body}
}}
"""

GCC_COMPILER = "/usr/bin/i686-linux-gnu-gcc"

ARM_GCC_COMPILER = "/usr/bin/arm-linux-gnueabi-gcc-8"


class CompilerError:

    def __init__(self, error_type, line_number, message):
        self.error_type = error_type
        self.line_number = line_number
        self.message = message

    def __hash__(self):
        return hash(self.line_number) + hash(self.message) + hash(self.error_type)

    def __eq__(self, other):
        return (
            self.error_type == other.error_type
            and self.line_number == other.line_number
            and self.message == other.message
        )
    
    def __repr__(self):
        return "{}('{}', {}, '{}')".format(
            self.__class__.__name__,
            self.error_type,
            self.line_number,
            self.message
        )

class Compiler(abc.ABC):
    """Abstract compiler class."""

    def __init__(self, output):
        self.output = output

    @classmethod
    def get_source(cls, expression=None, body=None, program=None):
        if expression:
            return TEMPLATE_EXPRESSION.format(expression=expression)
        elif body:
            return TEMPLATE_BODY.format(body=body)
        elif program:
            return program
        else:
            raise ValueError("Missing expression, body, or program argument.")
        
    def compile_and_run(self, expression=None, body=None, program=None):
        """Compile a source program and check the result.

        The test source code can be a single expression, or a statement block
        main, or an entire program.
        :param expression: Expression evaluated within main.
        :param body: code block executed within main. Must include return.
        :param progra: entire program, must include main.

        Exactly on of expression, body, or program must be given. Raises
        AssertionFailure if the compiled code returns non-zero.
        """
        source = self.get_source(expression, body, program)
        print(f"Compiling source program: {source}")

        self.compile(source, self.output)
        subprocess.run(self.output, check=True)
    
    def expression(self, expression, expected_errors=None):
        if expected_errors:
            self.error_check(self.get_source(expression=expression), expected_errors)
        else:
            self.compile_and_run(expression=expression)

    def body(self, body, expected_errors=None):
        if expected_errors:
            self.error_check(self.get_source(body=body), expected_errors)
        else:
            self.compile_and_run(body=body)

    def program(self, program, expected_errors=None):
        if expected_errors:
            self.error_check(self.get_source(program=program), expected_errors)
        else:
            self.compile_and_run(program=program)

    @abc.abstractmethod
    def compile(self, source, output):
        """Compile a source program.

        This should be defined in compiler-specific subclasses.
        """

    @abc.abstractmethod
    def error_check(self, source, expected_errors):
        """Check for errors in the source input only.

        This should be defined in compiler-specific subclasses.
        This should raise an AssertionError if the compiler reports no errors.

        expected_errors is a list of 
        """


class GccCompiler(Compiler):
    """GCC Compiler class

    This class provides a baseline for verifying the tests.
    """

    def compile(self, source, output):
        cmd = [GCC_COMPILER, "-x", "c", "-o", output, "-"]
        subprocess.run(cmd, input=source.encode(), check=True)
    
    def error_check(self, source, errors):
        cmd = [GCC_COMPILER, "-Werror", "-x", "c", "-S", "-o", "-", "-"]
        result = subprocess.run(cmd, input=source.encode(), check=False)

        if result.returncode == 0:
            raise AssertionError


class AccIrCompiler(Compiler):
    """ACC (IR-only) compiler."""

    def __init__(self, path, output, regalloc=False):
        super().__init__(output)
        self._path = path
        self._regalloc = regalloc

    def compile(self, source, output):
        if self._regalloc:
            cmd = [self._path, "-i", "-", "-"]
        else:
            cmd = [self._path, "-i", "-", "-r", "-"]
        ir = subprocess.run(cmd, input=source.encode(), check=True, capture_output=True)

        gcc_cmd = [GCC_COMPILER, "-x", "c", "-o", output, "-"]
        subprocess.run(gcc_cmd, input=ir.stdout, check=True)
    
    def error_check(self, source, expected_errors):
        raise NotImplemented


class AccAsmCompiler(Compiler):
    """ACC assembly-output compiler.

    Generates ARM assembly. Should be assembled with:
    arm-linux-gnueabi-gcc-8
    """
    def __init__(self, path, output):
        self._path = path
        self._output = output
    
    def compile(self, source, output):
        cmd = [self._path, '-', '-']
        acc_proc = subprocess.run(cmd, input=source.encode(), check=True, capture_output=True)

        cmd = [ARM_GCC_COMPILER, '-march=armv8-a', '-x', 'assembler', '-nostdlib', '-o', output, '-']
        subprocess.run(cmd, input=acc_proc.stdout, check=True)
    
    def compile_and_run(self, expression=None, body=None, program=None):
        src = self.get_source(expression, body, program)
        print(f"Compiling source program: {src}")
        self.compile(src, self._output)

        print(f"Running compiled program: {self._output}")
        subprocess.run([self._output], check=True)

    def error_check(self, source, expected_errors):
        raise NotImplemented

class AccCheckOnlyCompiler(Compiler):
    
    def __init__(self, path, output):
        super().__init__(output)
        self.path = path
    
    def compile(self, source, output):
        raise NotImplemented
    
    def error_check(self, source, expected_errors):
        cmd = [self.path, "-c", "-j", "-"]
        result = subprocess.run(cmd, input=source.encode(), capture_output=True)

        print(result.returncode)
        if result.returncode == 0:
            raise AssertionError
        
        if result.stdout.strip():
            json_errors = json.loads(result.stdout)['errors']
        else:
            json_errors = list()
        

        errors = set(list(CompilerError(**e) for e in json_errors))
        assert set(expected_errors) <= errors