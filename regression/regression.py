"""Regression test suite for ACC.

This file parses a C source file in regression/, and validates
the result of ACC (either error messages, or exit code from running the
compiled program) against metadata in the file.

Metadata in the file includes either expected error messages,
or the expected result of running the compiled program:

Expected error messages are encoded as:
// !error [SCANNER|PARSER|ANALYSIS] "[message]"

If no error messages are included, the source file should pass through
acc and compile. The expected exit code when the executable is run is
encoded as:
// !exit 0

Usage:
./regression.py --acc [path to acc] [SOURCE FILE]

E.g.
./regression.py --acc ../build/acc errors.c
"""
import re
import json
import subprocess
from argparse import ArgumentParser

IR_COMPILER = "i686-linux-gnu-gcc"


class AccError:
    """Class representing an error in ACC"""

    def __init__(self, error_type, line_number, message):
        self.error_type = error_type
        self.line = line_number
        self.message = message

    def __str__(self):
        return f"{self.error_type} ({self.line}): {self.message}"

    def __hash__(self):
        return hash(self.line) + hash(self.message) + hash(self.error_type)

    def __eq__(self, other):
        return (
            self.error_type == other.error_type
            and self.line == other.line
            and self.message == other.message
        )


class SourceFile:
    """Class representing the input source file"""

    def __init__(self, path):
        self.path = path
        self.source = open(path, "r").read()
        self.errors = self._get_expected_errors(self.source)
        self.expected_exit_code = 0

    @classmethod
    def _get_expected_errors(self, source_file):
        errors = set()
        source_lines = list(enumerate(source_file.split("\n")))
        prev_non_error_line = None
        for line_no, line in reversed(source_lines):
            if "!error" in line:
                expected_error_line = prev_non_error_line + 1

            elif "!exit" in line:
                self.expected_exit_code = int(line.split[5:].strip())
                continue

            else:
                prev_non_error_line = line_no
                continue

            component, message = re.search('error ([A-Z]+) "(.*)"', line).groups()
            errors.add(
                AccError(
                    error_type=component,
                    line_number=expected_error_line,
                    message=message,
                )
            )
        return errors


class Acc:
    """Class representing the acc compiler"""

    def __init__(self, path):
        self._path = path

    def run(self, args):
        output = subprocess.run(args, capture_output=True).stdout

        if output.strip():
            json_errors = json.loads(output)["errors"]
        else:
            json_errors = list()

        return set(list((AccError(**e) for e in json_errors)))

    def check(self, source_path):
        args = [self._path, "-j", "-c", source_path]
        return self.run(args)

    def compile(self, source_path, ir_output=None):
        args = [self._path, "-j", "-i", ir_output, source_path]
        return self.run(args)


def compile_expect_errors(acc, source_file):
    # Run the compiler.
    compiler_errors = acc.check(source_file.path)
    expected_errors = source_file.errors

    # Missing errors that did not occur
    missing_errors = expected_errors - compiler_errors

    # Unexpected errors that occurred but were not expected
    unexpected_errors = compiler_errors - expected_errors

    if missing_errors:
        print("Missing errors:")
        print("\n".join(str(e) for e in missing_errors))

    if unexpected_errors:
        print("Unexpected errors:")
        print("\n".join(str(e) for e in unexpected_errors))

    if missing_errors or unexpected_errors:
        print(
            (
                "There were disrepencies between the expected errors and "
                f"actual errors reported by the compiler in {source_file.path}"
            )
        )
        return False
    else:
        print(f"No unexpected or missing errors")
        return True


def compile_expect_no_errors(acc, source_file):
    ir_file = source_file.path[:-2] + ".ir.c"
    exe_file = source_file.path[:-2] + ".out"

    errors = acc.compile(source_file.path, ir_output=ir_file)

    if len(errors) != 0:
        printf(f"Unable to compile {source_file.path}, {len(errors)} errors reported)")
        return False

    compile_ir_cmd = [IR_COMPILER, "-o", exe_file, ir_file]
    if subprocess.run(compile_ir_cmd, stderr=subprocess.DEVNULL).returncode != 0:
        print(f"Unable to compile IR code: {ir_file}, errors reported by {IR_COMPILER}")
        return False

    # Run the compiled program.
    exitcode = subprocess.run(exe_file).returncode

    if exitcode != source_file.expected_exit_code:
        print(f"Exitcode was {exitcode}, expected {source_file.expected_exit_code}")
        return False
    else:
        print(f"Source file {source_file.path} compiled and passed okay")
        return True


def main():
    argparser = ArgumentParser()
    argparser.add_argument(
        "--acc", help="path to acc binary", required=True, action="store"
    )
    argparser.add_argument("source", help="source file", nargs="+")
    args = argparser.parse_args()

    errors = False

    for source_file in args.source:
        if re.match("^.*\.ir\.c$", source_file):
            continue

        print(f"\n**Processing: {source_file}**")

        acc = Acc(args.acc)
        source_file = SourceFile(source_file)

        if source_file.errors:
            print("Expecting errors")
            if not compile_expect_errors(acc, source_file):
                errors = True
        else:
            print("Expecting no errors")
            if not compile_expect_no_errors(acc, source_file):
                errors = True

    return not errors


if __name__ == "__main__":
    exit(0 if main() else 1)
