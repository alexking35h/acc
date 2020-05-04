#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "parser.h"
#include "pretty_print.h"
#include "scanner.h"
#include "error.h"
#include "analysis.h"
#include "symbol.h"

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
void Error_report_error(ErrorType, int, const char *) __attribute__((weak));

static bool json_stdout;

struct CommandLineArgs_t {
  const char* source_file;
  _Bool interactive;
  _Bool json;
};

/* Print error message to stdout in human-readable form. */
static void print_error_terminal(ErrorType type, int l, const char* msg) {
  printf("Error occurred ");
  switch (type) {
    case SCANNER:
      printf("in Scanner:\n");
      break;
    case PARSER:
      printf("in Parser:\n");
      break;
    default:
      printf(":\n");
      break;
  }
  printf(" > Line (%d): %s\n\n", l, msg);
}

/* Print error message to stdout in json */
static void print_error_json(ErrorType type, int l, const char* msg) {
  static int first = 0;
  if(first++) printf(",");
  printf("\n    {\"error_type\":");
  switch(type) {
    case SCANNER:
      printf("\"SCANNER\"");
      break;
    case PARSER:
      printf("\"PARSER\"");
      break;
    case ANALYSIS:
      printf("\"ANALYSIS\"");
      break;
    default:
      printf("null");
      break;
  }
  printf(", \"line_number\": %d, ", l);
  printf("\"message\": \"%s\"", msg);
  printf("}");
}

void Error_report_error(ErrorType error_type, int line_number, const char *msg) {
  if(json_stdout) {
    print_error_json(error_type, line_number, msg);
  } else {
    print_error_terminal(error_type, line_number, msg);
  }
}

static void print_help() {
  printf("ACC Version %d.%d.%d\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
  printf("\nUsage: acc [OPTIONS] [FILE]\n\n");
  printf("Options:\n");
  printf(" -v print version\n");
  printf(" -h print help\n");
  printf("\n");
  printf(
      "If [FILE] is omitted, acc runs in interactive mode, generating the "
      "AST\n");
  printf("for C source code passed to the command line\n");
}

static void print_version() {
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
static _Bool parse_cmd_args(int argc, char** argv, struct CommandLineArgs_t* args) {
  int c = 0;

  while ((c = getopt(argc, argv, "vhj")) != -1) {
    switch (c) {
      case 'h':
        print_help();
        return false;
      case 'v':
        print_version();
        return false;
      case 'j':
        args->json = true;
        break;
    }
  }

  if ((optind) == argc) {
    // Running in interactive mode if no arguments are provided.
    args->interactive = true;
  } else {
    // Read from file.
    args->interactive = false;
    args->source_file = argv[optind];
  }

  // JSON stdout can only be used in non-interactive mode.
  if(args->interactive && args->json) {
    printf("JSON stdout (-j) cannot be used in interactive mode (-i)\n");
    return false;
  }
  return true;
}

static DeclAstNode* get_ast(const char* source) {
  Scanner* scanner = Scanner_init(source);
  Parser* parser = Parser_init(scanner);

  // // Generate the AST for the file.
  DeclAstNode* ast = Parser_translation_unit(parser);

  // Context-sensitive analysis (semantic analysis).
  SymbolTable* global = symbol_table_create(NULL);
  analysis_ast_walk_decl(ast, global);

  return ast;
}

static const char* read_file(const char* file_path) {
  char* file_contents = malloc(1024 * sizeof(char));
  FILE* f = fopen(file_path, "r");

  fread(file_contents, sizeof(char), 1023, f);
  return file_contents;
}

int main(int argc, char** argv) {
  struct CommandLineArgs_t args = {};
  if (!parse_cmd_args(argc, argv, &args)) return 1;

  json_stdout = args.json;

  if (args.interactive) {
    printf("Running acc in interactive mode.\n");

    while (1) {
      char src[100], ast[300];

      printf("\n>> ");
      fgets(src, 99, stdin);

      DeclAstNode* decl = get_ast(src);
      if (!decl) continue;

      pretty_print_decl(decl, ast, 300);
      printf("%s\n", ast);
    }
  } else {
    const char* file = read_file(args.source_file);
    char ast[1000] = "";
    if(args.json)
      printf("{\n  \"errors\": [");

    DeclAstNode* decl = get_ast(file);

    if(args.json)
      printf("\n  ]\n}\n");

    // pretty_print_decl(decl, ast, 1000);
    // printf("%s\n", ast);
  }
}
