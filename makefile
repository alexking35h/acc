
GIT_COMMIT=$(shell git describe --always)
GIT_REPO=$(shell git remote get-url origin)

# Compiler flags
CFLAGS=-Wall -Iinclude $(shell pkg-config --libs --cflags cmocka) -g -DGIT_COMMIT=\"$(GIT_COMMIT)\" -DGIT_REPO=\"$(GIT_REPO)\"
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

.PHONY: test
.PHONY: format
.PHONY: build

test: build build/test_scanner build/test_parser_expression build/test_parser_declaration build/test_parser_statement build/test_parser_error
	build/test_scanner
	build/test_parser_expression
	build/test_parser_declaration
	build/test_parser_statement
	build/test_parser_error

build/test_scanner: $(ACC_OBJECTS) build/test_scanner.o build/test.o
	$(CC) $^ -o $@ $(CFLAGS) -Wl,--wrap=Error_report_error -Wl,--wrap=Error_report_warning

build/test_parser_expression: $(ACC_OBJECTS) build/test_parser_expression.o build/test.o
	$(CC) $^ -o $@ $(CFLAGS) 

build/test_parser_declaration: $(ACC_OBJECTS) build/test_parser_declaration.o build/test.o
	$(CC) $^ -o $@ $(CFLAGS) 

build/test_parser_statement: $(ACC_OBJECTS) build/test_parser_statement.o build/test.o
	$(CC) $^ -o $@ $(CFLAGS) 

build/test_parser_error: $(ACC_OBJECTS) build/test_parser_error.o build/test.o
	$(CC) $^ -o $@ $(CFLAGS)

build/acc: $(ACC_OBJECTS)
	$(CC) $^ -o $@ $(CFLAGS)

$(ACC_OBJECTS): build/%.o: source/%.c build
	$(CC) -c $< -o $@ $(CFLAGS)

$(TEST_OBJECTS): build/%.o: test/%.c build
	$(CC) -c $< -o $@ $(CFLAGS)

format:
	clang-format --style=Google -i include/*.h source/*.c test/*.c

$(BUILD_DIR):
	mkdir -p build

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
