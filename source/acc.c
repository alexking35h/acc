#include "parser.h"
#include "pretty_print.h"
#include "scanner.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#ifndef VERSION_MAJOR
#define VERSION_MAJOR 0
#define VERSION_MINOR 0
#define VERSION_PATCH 0
#endif

#ifndef GIT_COMMIT
#define GIT_COMMIT "0000000"
#endif

#ifndef GIT_REPO
#define GIT_REPO ""
#endif

int main(int, char**) __attribute__((weak));

struct CommandLineArgs_t {
  const char* source_file;
};

void print_help() {
  printf("ACC Version %d.%d.%d\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
  printf("\nUsage: acc [OPTIONS] [FILE]\n\n");
  printf("Options:\n");
  printf(" -v print version\n");
  printf(" -h print help\n");
}

void print_version() {
  printf("ACC Version %d.%d.%d\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
  printf("Compiled: %s %s\n", __DATE__, __TIME__);
  printf("Git repository: %s\n", GIT_REPO);
  printf("Git commit: %s\n", GIT_COMMIT);
}

/*
 * Parse command line options/arguments.
 *
 * Usage: acc [options] [source file]
 *
 * options:
 * -v  print version information
 * -h  print help information
 *
 * Returns true if the program should continue.
 * Quit otherwise.
 */
_Bool parse_cmd_args(int argc, char** argv, struct CommandLineArgs_t* args) {
  int c = 0;

  while ((c = getopt(argc, argv, "vh")) != -1) {
    switch (c) {
      case 'h':
        print_help();
        return false;
      case 'v':
        print_version();
        return false;
    }
  }

  if (optind == (argc - 1)) {
    args->source_file = argv[optind];
    return true;
  }
  return false;
}

const char* read_file(const char* file_path) {
  char* file_contents = malloc(256 * sizeof(char));
  FILE* f = fopen(file_path, "r");

  fread(file_contents, sizeof(char), 253, f);
  return file_contents;
}

int main(int argc, char** argv) {
  struct CommandLineArgs_t q = {};
  if (!parse_cmd_args(argc, argv, &q)) return 1;

  // Let's get compiling!
  const char* file = read_file(q.source_file);

  Scanner* scanner = Scanner_init(file, NULL);
  Parser* parser = Parser_init(scanner, NULL);

  // Generate the AST for the file.
  DeclAstNode* decl = Parser_declaration(parser);

  char pretty_print[1024];
  pretty_print_decl(decl, pretty_print, 1024);

  printf("%s\n", pretty_print);
}
