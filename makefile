
# Compiler flags
CFLAGS=-Wall -Iinclude $(shell pkg-config --libs --cflags cmocka) -g
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

OBJECTS = $(ACC_OBJECTS) $(TEST_OBJECTS)

.PHONY: test
.PHONY: format

test: build build/test_scanner build/test_parser_expression 
	build/test_scanner
	build/test_parser_expression

build/test_scanner: $(ACC_OBJECTS) build/test_scanner.o
	$(CC) $^ -o $@ $(CFLAGS) -Wl,--wrap=Error_report_error -Wl,--wrap=Error_report_warning

build/test_parser_expression: $(ACC_OBJECTS) build/test_parser_expression.o
	$(CC) $^ -o $@ $(CFLAGS) 

build/test_parser_declaration: $(ACC_OBJECTS) build/test_parser_declaration.o
	$(CC) $^ -o $@ $(CFLAGS) 

docker_build:
	docker build --tag acc:v1 .

docker_run:
	docker run -it --rm -v$(PWD):/home/ acc:v1 bash

$(ACC_OBJECTS): build/%.o: source/%.c
	$(CC) -c $< -o $@ $(CFLAGS)

$(TEST_OBJECTS): build/%.o: test/%.c
	$(CC) -c $< -o $@ $(CFLAGS)

format:
	clang-format --style=Google -i include/*.h source/*.c test/*.c

build:
	mkdir -p build

clean:
	rm -rf build
	
