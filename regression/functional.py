import abc
import subprocess
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


class Compiler(abc.ABC):
    """Abstract compiler class."""

    def __init__(self, output):
        self.output = output

    @classmethod
    def get_source(cls, expression, body, program):
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

    def expression(self, expression):
        self.compile_and_run(expression=expression)

    def body(self, body):
        self.compile_and_run(body=body)

    def program(self, program):
        self.compile_and_run(program=program)

    @abc.abstractmethod
    def compile(self, source, output):
        """Compile a source program.

        This should be defined in compiler-specific subclasses.
        """


class GccCompiler(Compiler):
    """GCC Compiler class

    This class provides a baseline for verifying the tests.
    """

    def compile(self, source, output):
        cmd = [GCC_COMPILER, "-x", "c", "-o", output, "-"]
        subprocess.run(cmd, input=source.encode(), check=True)


class AccIrCompiler(Compiler):
    """ACC (IR-only) compiler."""

    def __init__(self, path, output):
        super().__init__(output)
        self._path = path

    def compile(self, source, output):
        cmd = [self._path, "-"]
        ir = subprocess.run(cmd, input=source.encode(), check=True, capture_output=True)

        gcc_cmd = [GCC_COMPILER, "-x", "c", "-o", output, "-"]
        subprocess.run(gcc_cmd, input=ir.stdout, check=True)
