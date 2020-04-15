//#include "parser.h"
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>

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

struct CommandLineArgs_t {
  const char * source_file;
};

void print_help(){}

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
_Bool parse_cmd_args(int argc, char** argv, struct CommandLineArgs_t * args) {
  int c = 0;

  while((c = getopt(argc, argv, "vh")) != -1) {
    switch(c) {
      case 'h':
        print_help();
        return false;
      case 'v':
        print_version();
        return false;
    }
  }

  if(optind == (argc-1)) {
    args->source_file = argv[optind];
    return true;
  }

  printf("ploop\n");
  return false;
}

int main(int argc, char** argv) {
  struct CommandLineArgs_t q = {};
  parse_cmd_args(argc, argv, &q);
  printf("%s\n", q.source_file);
}
  

  

  

  
  