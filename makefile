
# Compiler flags
CFLAGS=-Wall -Iinclude $(shell pkg-config --libs --cflags check)
LDFLAGS=
CC=gcc

SOURCE_DIR=source
TEST_DIR=test
BUILD_DIR=build

# Source files
ACC_SOURCES=$(wildcard $(SOURCE_DIR)/*.c)
TEST_SOURCES=$(wildcard $(TEST_DIR)/*.c)

# Object files
ACC_OBJECTS=$(patsubst $(SOURCE_DIR)/%.c, $(BUILD_DIR)/%.o, $(ACC_SOURCES))
TEST_OBJECTS=$(patsubst $(TEST_DIR)/%.c, $(BUILD_DIR)/%.o, $(TEST_SOURCES))

OBJECTS = $(ACC_OBJECTS) $(TEST_OBJECTS)

.PHONY: test
.PHONY: all

all: test acc

test: build build/test_acc
	./build/test_acc

acc: build build/acc

build:
	mkdir -p build

build/acc: $(ACC_OBJECTS)
	$(CC) $^ -o $@ $(CFLAGS) 

build/test_acc: $(ACC_OBJECTS) $(TEST_OBJECTS)
	$(CC) $^ -o $@ $(CFLAGS) 

$(ACC_OBJECTS): build/%.o: source/%.c
	$(CC) -c $< -o $@ $(CFLAGS)

$(TEST_OBJECTS): build/%.o: test/%.c
	$(CC) -c $< -o $@ $(CFLAGS)



