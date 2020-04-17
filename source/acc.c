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
  _Bool interactive;
};

void print_help() {
  printf("ACC Version %d.%d.%d\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
  printf("\nUsage: acc [OPTIONS] [FILE]\n\n");
  printf("Options:\n");
  printf(" -v print version\n");
  printf(" -h print help\n");
  printf("\n");
  printf("If [FILE] is omitted, acc runs in interactive mode, generating the AST\n");
  printf("for C source code passed to the command line\n");
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

  if ((optind) == argc) {
    // Running in interactive mode if no arguments are provided.
    args->interactive = true;
  }
  else {
    // Read from file.
    args->interactive = false;
    args->source_file = argv[optind];
  }
  return true;
}

DeclAstNode* get_ast(const char * source) {
  Scanner* scanner = Scanner_init(source, NULL);
  Parser* parser = Parser_init(scanner, NULL);

  // Generate the AST for the file.
  return Parser_declaration(parser);
}

const char* read_file(const char* file_path) {
  char* file_contents = malloc(256 * sizeof(char));
  FILE* f = fopen(file_path, "r");

  fread(file_contents, sizeof(char), 253, f);
  return file_contents;
}

int main(int argc, char** argv) {
  struct CommandLineArgs_t args = {};
  if (!parse_cmd_args(argc, argv, &args)) return 1;

  if(args.interactive) {
    printf("Running acc in interactive mode.\n");

    while (1) {
      char src[100], ast[300];

      printf("\n>> ");
      fgets(src, 99, stdin);

      DeclAstNode* decl = get_ast(src);
      pretty_print_decl(decl, ast, 300);
      printf("%s\n", ast);
    }
  }
  else {
    const char* file = read_file(args.source_file);
    char ast[1000];

    DeclAstNode* decl = get_ast(file);
    pretty_print_decl(decl, ast, 1000);
    printf("%s\n", ast);
  }
}
