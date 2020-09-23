import pytest
import re
import subprocess
import json
import os
import tempfile

ACC_PATH=os.environ.get("ACC_PATH", os.path.join(os.path.dirname(__file__), "../build/acc"))

def test_help():
    """Passing -h with no other arguments should return 0, and do nothing."""
    assert subprocess.run([ACC_PATH, '-h']).returncode == 0

def test_version():
    """Passing -v should print out version information."""
    proc = subprocess.run([ACC_PATH, '-v'], capture_output=True)
    assert re.search(r'Version: \d.\d.\d', proc.stdout.decode())
    assert proc.returncode == 0

def test_json():
    """Passing -j should print out errors in json."""
    proc = subprocess.run([ACC_PATH, '-j', '-'], capture_output=True, input="?;".encode())
    print(proc.stdout.decode())
    assert "errors" in json.loads(proc.stdout.decode())

def test_check():
    """Passing -c should check the source input only (no output)."""
    src = "int main(){}"
    proc = subprocess.run([ACC_PATH, '-c', '-'], capture_output=True, input=src.encode())
    assert proc.returncode == 0
    assert proc.stdout.decode().strip() == ""

def test_invalid_omit_arguments():
    """Invalid arguments: -r should only be used with -i."""
    proc = subprocess.run([ACC_PATH, '-r', '-'])
    assert proc.returncode == 1

def test_intermediate_output_stdout():
    """Passing -i - should generate intermediate output to stdout."""
    src = "int main(){}"
    proc = subprocess.run([ACC_PATH, '-i', '-', '-'], capture_output=True, input=src.encode())
    assert proc.returncode == 0
    assert proc.stdout.decode().startswith("// === ACC IR ===")

def test_intermediate_output_file():
    """Passing -i [FILE] should write intermediate output to file."""
    src = "int main(){}"
    with tempfile.NamedTemporaryFile() as temp:
        proc = subprocess.run([ACC_PATH, '-i', temp.name, '-'], input=src.encode())
        assert proc.returncode == 0
        assert open(temp.name, 'r').read().startswith("// === ACC IR ===")

def test_file_input():
    """Test reading input from file (not '-')."""
    with tempfile.NamedTemporaryFile() as temp:
        with open(temp.name, 'w') as temp_file:
            temp_file.write("int main(){}")
        
        proc = subprocess.run([ACC_PATH, temp.name], capture_output=True)
        assert proc.returncode == 0
        assert proc.stdout.decode().startswith("// === ACC IR ===")