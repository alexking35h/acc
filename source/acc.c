#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "analysis.h"
#include "error.h"
#include "parser.h"
#include "pretty_print.h"
#include "scanner.h"
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

extern int errno;

#define BANNER \
    "     ___       ______   ______ \n"     \
    "    /   \\     /      | /      |\n"     \
    "   /  ^  \\   |  ,----'|  ,----'\n"     \
    "  /  /_\\  \\  |  |     |  |     \n"     \
    " /  _____  \\ |  `----.|  `----.\n"     \
    "/__/     \\__\\ \\______| \\______|\n"


int main(int, char **) __attribute__((weak));

typedef struct CommandLineArgs_t {
    const char *source_file;
    _Bool json;
} CommandLineArgs;

typedef struct AccCompiler_t {
    ErrorReporter * error_reporter;
    Scanner * scanner;
    Parser * parser;
} AccCompiler;

// Print help information.
static void help(const char * exe_path) {
    printf("ACC (%d.%d.%d)\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
    printf("\nUsage: %s [OPTIONS] [FILE]\n\n", exe_path);
    printf("Options:\n");
    printf("  -v version information\n");
    printf("  -h help\n");
    printf("  -j json output\n");
    printf("\n");
    printf("[FILE] is a file path to the C source file which will be compiled\n");
    printf("Returns 0 if no errors were reported\n");
}

// Print version information
static void version() {
    printf("%s\n", BANNER);
    printf("Alex's C Compiler\n");
    printf("Version: %d.%d.%d\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
    printf("Compiled: %s %s\n", __DATE__, __TIME__);
    printf("Git repository: %s\n", GIT_REPO);
    printf("Git hash: %s\n", GIT_COMMIT);
}

/*
 * Parse command line options/arguments.
 *
 * Usage: acc [options] [source file]
 *
 * options:
 * -v  print version information
 * -j  print output in json format
 * -h  print help information
 *
 * Returns true if the program should continue.
 * Quit otherwise.
 */
static _Bool parse_cmd_args(int argc, char **argv, struct CommandLineArgs_t *args)
{
    int c = 0;
    args->json = false;

    while ((c = getopt(argc, argv, "vhj")) != -1)
    {
        switch (c)
        {
        case 'h':
            help(argv[0]);
            return false;
        case 'v':
            version();
            return false;
        case 'j':
            args->json = true;
            break;
        }
    }

    if ((optind) == argc)
    {
        printf("No source file provided. See help (-h)\n");
        return false;
    }
    args->source_file = argv[optind];
    return true;
}

/*
 * Read the source input into a character array.
 */
static const char * read_source(const char * path) {
    FILE *f = fopen(path, "r");
    if(!f) {
        goto err;
    }

    // Get the file size.
    if(fseek(f, 0, SEEK_END)) {
        goto err;
    }
    int fsize = ftell(f);
    if(fseek(f, 0, SEEK_SET)) {
        goto err;
    }

    // Allocate storage space on the heap for the file.
    char * file_contents = malloc(fsize);
    if(!file_contents) {
        printf("Unable to allocate sufficient space for the file\n");
        return NULL;
    }

    // Read the entire file.
    if(fsize != fread(file_contents, 1, fsize, f)) goto err;
    fclose(f);

    return file_contents;
err:
    if(file_contents) free(file_contents);
    printf("Unable to read source file:\n%s\n", strerror(errno));
    return NULL;
}

static AccCompiler * compiler_init(const char * path) {
    const char * src = read_source(path);
    if(!src) return NULL;

    AccCompiler * compiler = calloc(1, sizeof(AccCompiler));
    compiler->error_reporter = Error_init();
    compiler->scanner = Scanner_init(src, compiler->error_reporter);
    compiler->parser = Parser_init(compiler->scanner, compiler->error_reporter);

    return compiler;
}

static DeclAstNode * compiler_parse(AccCompiler * compiler) {
    return Parser_translation_unit(compiler->parser);
}

static void compiler_analysis(AccCompiler * compiler, DeclAstNode * ast_root) {
    
}

static void print_error_json(ErrorType type, int line, int pos, char * title, char *desc) {

}

static void print_error_commandline(ErrorType type, char * line, int line_number, int pos, char * title, char *desc) {
    int line_len = strchr(line, '\n') - line;
    char * line_cpy = malloc(line_len+1);
    strncpy(line_cpy, line, line_len);
    line_cpy[line_len] = '\0';

    // Print out the Error! :-)
    printf("\nError occurred on line %d ", line_number);
    switch(type) {
        case SCANNER:
            printf("(scanner)");
            break;
        case PARSER:
            printf("(parser)");
            break;
        case ANALYSIS:
            printf("(analysis)");
            break;
    }

    printf("\nError: %s\n", title);
    
    if(desc) {
        printf("%s", desc);
    }

    // Print out the source line, and arrow.
    printf(" > %s\n", line_cpy);

    for(int i = 0;i < pos+3;i++) printf(" ");
    printf("^\n");
}

static void compiler_print_errors(AccCompiler *compiler, _Bool json) {
    int beginning = 0;
    ErrorType type;
    int line_number;
    int line_position;
    char * title;
    char * description;

    for(;;) {
        if(!Error_get_errors(
            compiler->error_reporter,
            &type,
            &line_number,
            &line_position,
            &title,
            &description,
            beginning++ == 0
        )) break;

        if(json) {
            print_error_json(type, line_number, line_position, title, description);
        } else {
            const char * line = Scanner_get_line(compiler->scanner, line_number-1);
            print_error_commandline(type, line, line_number, line_position, title, description);
        }
    }
}

static void compiler_destroy(AccCompiler * compiler) {
    Error_destroy(compiler->error_reporter);
    Parser_destroy(compiler->parser);
    Scanner_destroy(compiler->scanner);
}

int main(int argc, char **argv)
{
    CommandLineArgs args;
    AccCompiler * compiler;
    if (!parse_cmd_args(argc, argv, &args))
        return 1;

    if(!(compiler = compiler_init(args.source_file))) {
        return 1;
    }

    // Generate the AST, from the source input.
    DeclAstNode * ast_root = compiler_parse(compiler);

    // Context-sensitive analysis on the AST.
    compiler_analysis(compiler, ast_root);

    // Check if errors occurred during scanning/parsing/analysis.
    // Abort if we cannot proceed.
    if(Error_has_errors(compiler->error_reporter)) {
        compiler_print_errors(compiler, args.json);
        return 1;
    }
}
