
GIT_COMMIT=$(shell git describe --always)
GIT_REPO=$(shell git remote get-url origin)

# Compiler flags
CFLAGS=-Wno-switch-Wall -Iinclude $(shell pkg-config --libs --cflags cmocka) -g -DGIT_COMMIT=\"$(GIT_COMMIT)\" -DGIT_REPO=\"$(GIT_REPO)\"
LDFLAGS=
CC=gcc

SOURCE_DIR=source
TEST_DIR=test
BUILD_DIR=build
PWD=$(shell pwd)

# Source files
ACC_SOURCES=$(wildcard $(SOURCE_DIR)/*.c)
TEST_SOURCES=$(wildcard $(TEST_DIR)/*.c)

# Object files
ACC_OBJECTS=$(patsubst $(SOURCE_DIR)/%.c, $(BUILD_DIR)/%.o, $(ACC_SOURCES))
TEST_OBJECTS=$(patsubst $(TEST_DIR)/%.c, $(BUILD_DIR)/%.o, $(TEST_SOURCES))

# Unit test executables
TEST_EXE=$(patsubst $(TEST_DIR)/%.c, $(BUILD_DIR)/%, $(wildcard $(TEST_DIR)/test_*.c))
ACC_EXE=$(BUILD_DIR)/acc

.PHONY: test
.PHONY: format
.PHONY: $(RUN_TESTS)

# Phony targets to run the unit tests
RUN_TESTS=$(addprefix run_, $(TEST_EXE))

test: $(RUN_TESTS)

$(RUN_TESTS): run_%:%
	./$<

build/test_scanner: $(ACC_OBJECTS) build/test_scanner.o build/test.o
	$(CC) $^ -o $@ $(CFLAGS) -Wl,--wrap=Error_report_error

build/test_parser_expression: $(ACC_OBJECTS) build/test_parser_expression.o build/test.o
	$(CC) $^ -o $@ $(CFLAGS)

build/test_parser_declaration: $(ACC_OBJECTS) build/test_parser_declaration.o build/test.o
	$(CC) $^ -o $@ $(CFLAGS) 

build/test_parser_statement: $(ACC_OBJECTS) build/test_parser_statement.o build/test.o
	$(CC) $^ -o $@ $(CFLAGS) 

build/test_parser_error: $(ACC_OBJECTS) build/test_parser_error.o build/test.o
	$(CC) $^ -o $@ $(CFLAGS)

build/test_symbol_table: $(ACC_OBJECTS) build/test_symbol_table.o
	$(CC) $^ -o $@ $(CFLAGS)

build/test_analysis_type_checking: $(ACC_OBJECTS) build/test_analysis_type_checking.o build/test.o
	$(CC) $^ -o $@ $(CFLAGS)

build/test_analysis_conversions: $(ACC_OBJECTS) build/test_analysis_conversions.o build/test.o
	$(CC) $^ -o $@ $(CFLAGS)

build/test_analysis_declarations: $(ACC_OBJECTS) build/test_analysis_declarations.o build/test.o
	$(CC) $^ -o $@ $(CFLAGS) -Wl,--wrap=symbol_table_get -Wl,--wrap=symbol_table_put -Wl,--wrap=symbol_table_create

regression_test: build/acc
	python3 regression/regression.py --acc $^ regression/*.c

build/acc: $(ACC_OBJECTS) | build
	$(CC) $^ -o $@ $(CFLAGS)

$(ACC_OBJECTS): build/%.o: source/%.c | build
	$(CC) -c $< -o $@ $(CFLAGS)

$(TEST_OBJECTS): build/%.o: test/%.c | build
	$(CC) -c $< -o $@ $(CFLAGS)

$(BUILD_DIR):
	mkdir -p build

format:
	clang-format --style=file -i include/*.h source/*.c test/*.c

clean:
	rm -rf build

docker_build:
	docker build --tag acc:v1 .

docker_sh:
	docker run -it --rm -v$(PWD):/home/ acc:v1 bash

docker_test:
	docker run --rm -v$(PWD):/home/ acc:v1 make test

docker_acc:
	docker run --rm -it -v$(PWD):/home/ acc:v1 bash -c "make build/acc; build/acc"
