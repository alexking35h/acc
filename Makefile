GIT_COMMIT=$(shell git describe --always)
GIT_REPO=$(shell git remote get-url origin)

# Compiler flags
CFLAGS=-Wno-switch -Wall -Iinclude $(shell pkg-config --libs --cflags cmocka) \
	   -g -DGIT_COMMIT=\"$(GIT_COMMIT)\" -DGIT_REPO=\"$(GIT_REPO)\"
CFLAGS_COVERAGE=--coverage
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
ACC_OBJECTS_COVERAGE=$(patsubst $(SOURCE_DIR)/%.c, $(BUILD_DIR)/cov/%.o, $(ACC_SOURCES))
TEST_OBJECTS=$(patsubst $(TEST_DIR)/%.c, $(BUILD_DIR)/%.o, $(TEST_SOURCES))

# Unit test executables
TEST_EXE=$(patsubst $(TEST_DIR)/%.c, $(BUILD_DIR)/%, $(wildcard $(TEST_DIR)/test_*.c))
ACC_EXE=$(BUILD_DIR)/acc

.PHONY: test
.PHONY: $(RUN_TESTS)
.PHONY: benchmark
.PHONY: functional

# Phony targets to run the unit tests
RUN_TESTS=$(addprefix run_, $(TEST_EXE))

ifeq ("$(shell test -e /.dockerenv && echo in_docker)", "in_docker")
# Running within Docker environment

build/test_scanner: $(ACC_OBJECTS_COVERAGE) build/test_scanner.o build/test.o
	$(CC) $^ -o $@ $(CFLAGS) $(CFLAGS_COVERAGE) -Wl,--wrap=Error_report_error

build/test_parser_expression: $(ACC_OBJECTS_COVERAGE) build/test_parser_expression.o build/test.o
	$(CC) $^ -o $@ $(CFLAGS) $(CFLAGS_COVERAGE)

build/test_parser_declaration: $(ACC_OBJECTS_COVERAGE) build/test_parser_declaration.o build/test.o
	$(CC) $^ -o $@ $(CFLAGS) $(CFLAGS_COVERAGE) 

build/test_parser_statement: $(ACC_OBJECTS_COVERAGE) build/test_parser_statement.o build/test.o
	$(CC) $^ -o $@ $(CFLAGS) $(CFLAGS_COVERAGE) 

build/test_parser_error: $(ACC_OBJECTS_COVERAGE) build/test_parser_error.o build/test.o
	$(CC) $^ -o $@ $(CFLAGS) $(CFLAGS_COVERAGE)

build/test_symbol_table: $(ACC_OBJECTS_COVERAGE) build/test_symbol_table.o
	$(CC) $^ -o $@ $(CFLAGS) $(CFLAGS_COVERAGE)

build/test_analysis_type_checking: $(ACC_OBJECTS_COVERAGE) build/test_analysis_type_checking.o build/test.o
	$(CC) $^ -o $@ $(CFLAGS) $(CFLAGS_COVERAGE)

build/test_analysis_conversions: $(ACC_OBJECTS_COVERAGE) build/test_analysis_conversions.o build/test.o
	$(CC) $^ -o $@ $(CFLAGS) $(CFLAGS_COVERAGE)

build/test_analysis_declarations: $(ACC_OBJECTS_COVERAGE) build/test_analysis_declarations.o build/test.o
	$(CC) $^ -o $@ $(CFLAGS) $(CFLAGS_COVERAGE) -Wl,--wrap=symbol_table_get -Wl,--wrap=symbol_table_put -Wl,--wrap=symbol_table_create

build/test_arch: $(ACC_OBJECTS_COVERAGE) build/test_arch.o
	$(CC) $^ -o $@ $(CFLAGS) $(CFLAGS_COVERAGE)

build/test_liveness: $(ACC_OBJECTS_COVERAGE) build/test_liveness.o
	$(CC) $^ -o $@ $(CFLAGS) $(CFLAGS_COVERAGE)

build/test_regalloc: $(ACC_OBJECTS_COVERAGE) build/test_regalloc.o
	$(CC) $^ -o $@ $(CFLAGS) $(CFLAGS_COVERAGE)

test: $(RUN_TESTS)

$(RUN_TESTS): run_%:%
	$<

functional_test: build/cov/acc
	pip3 install acctools/
	ACC_PATH=$^ pytest functional/

coverage:
	gcov -o build/cov/ source/*.c --no-output \
	| sed -n 's/.*uted:\([0-9][0-9]*\).*of \([0-9][0-9]*\)/\1 \2/p' \
	| awk '{t+=$$2;a+=(.01*$$1*$$2)}END{printf "\n** Test coverage: %f0.2%% **\n\n",100*a/t}'

benchmark: build/acc
	pip3 install acctools/
	ACC_PATH=$^ python3 benchmark/measure.py

$(ACC_OBJECTS): build/%.o: source/%.c | build
	$(CC) -c $< -o $@ $(CFLAGS)

build/acc: $(ACC_OBJECTS) | build
	$(CC) $^ -o $@ $(CFLAGS)

$(ACC_OBJECTS_COVERAGE): build/cov/%.o: source/%.c | build
	$(CC) -c $< -o $@ $(CFLAGS) $(CFLAGS_COVERAGE)

build/cov/acc: $(ACC_OBJECTS_COVERAGE) | build
	$(CC) $^ -o $@ $(CFLAGS) $(CFLAGS_COVERAGE)

$(TEST_OBJECTS): build/%.o: test/%.c | build
	$(CC) -c $< -o $@ $(CFLAGS)

$(BUILD_DIR):
	mkdir -p build/cov/

else
# Not running in a Docker environment
# Most build rules should be handled by starting a new Docker environment
# and runing the command within that.

docker_build:
	docker build --tag acc:v1 .

docker_sh:
	docker run -it --rm --user $(shell id -u):$(shell id -g) -v$(PWD):/home/ acc:v1 bash

benchmark test:
	docker run -it --rm --user $(shell id -u):$(shell id -g) -v$(PWD):/home/ make -f Makefile $@
%:
	docker run -it --rm --user $(shell id -u):$(shell id -g) -v$(PWD):/home/ acc:v1 make -f Makefile $@

endif

clean:
	rm -rf build
