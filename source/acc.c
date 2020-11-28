#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "analysis.h"
#include "error.h"
#include "ir.h"
#include "ir_gen.h"
#include "parser.h"
#include "pretty_print.h"
#include "scanner.h"
#include "symbol.h"
#include "liveness.h"
#include "regalloc.h"

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

#define BANNER                                                                           \
    "     ___       ______   ______ \n"                                                  \
    "    /   \\     /      | /      |\n"                                                 \
    "   /  ^  \\   |  ,----'|  ,----'\n"                                                 \
    "  /  /_\\  \\  |  |     |  |     \n"                                                \
    " /  _____  \\ |  `----.|  `----.\n"                                                 \
    "/__/     \\__\\ \\______| \\______|\n"

#define STDIN_READ_CHUNK_SIZE 256

extern int errno;

// main is defined as a weak symbol so that individual unit test files
// can override it.
int main(int, char **) __attribute__((weak));

void assembly_gen(FILE * fd, IrFunction * program);


typedef struct CommandLineArgs_t
{
    const char *source_file;
    _Bool json;
    _Bool check_only;
    _Bool omit_regalloc;
    const char *ir_output;
} CommandLineArgs;

typedef struct AccCompiler_t
{
    char *source;
    ErrorReporter *error_reporter;
    Scanner *scanner;
    Parser *parser;
} AccCompiler;

static void help(const char *exe_path)
{
    printf("ACC (%d.%d.%d)\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
    printf("\nUsage: %s [OPTIONS] [FILE]\n\n", exe_path);
    printf("Options:\n");
    printf("  -v version information\n");
    printf("  -h help\n");
    printf("  -j json output\n");
    printf("  -c check only (do not compile)\n");
    printf("  -i [FILE] Save Intermediate Representation (IR) output to file\n");
    printf("  -r omit register allocation (use virtual register allocations)\n");
    printf("\n");
    printf("[FILE] is a file path to the C source file which will be compiled\n");
    printf("(use '-' to read from stdin).\n\n");
    printf("Returns 0 if no errors were reported\n");
}

static void version()
{
    printf("%s\n", BANNER);
    printf("Alex's C Compiler\n");
    printf("Version: %d.%d.%d\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
    printf("Compiled: %s %s\n", __DATE__, __TIME__);
    printf("Git repository: %s\n", GIT_REPO);
    printf("Git hash: %s\n", GIT_COMMIT);
}

/*
 * Parse command line options/arguments.
 */
static _Bool parse_cmd_args(int argc, char **argv, struct CommandLineArgs_t *args)
{
    int c = 0;
    args->json = false;
    args->check_only = false;
    args->omit_regalloc = false;

    while ((c = getopt(argc, argv, "rvhjci:")) != -1)
    {
        switch (c)
        {
        case 'r':
            args->omit_regalloc = true;
            break;
        case 'h':
            help(argv[0]);
            exit(0);
        case 'v':
            version();
            exit(0);
        case 'j':
            args->json = true;
            break;
        case 'c':
            args->check_only = true;
            break;
        case 'i':
            args->ir_output = optarg;
            break;
        case '?':
            help(argv[0]);
            exit(1);
        }
    }

    if ((optind) == argc)
    {
        printf("No source file provided. See help (-h)\n");
        exit(1);
    }
    if (args->omit_regalloc && !args->ir_output)
    {
        printf("-r must be used with -i (IR output must be used if not allocating "
               "registers)\n");
        exit(1);
    }
    args->source_file = argv[optind];
}

static char *read_source_stdin()
{
    // Read from stdin in 256-byte chunks.
    char *source_buf = malloc(STDIN_READ_CHUNK_SIZE + 1);

    for (int read_offset = 0;;)
    {
        int read_size = fread(source_buf + read_offset, 1, STDIN_READ_CHUNK_SIZE, stdin);
        if (read_size != STDIN_READ_CHUNK_SIZE)
        {
            source_buf[read_offset + read_size] = '\0';
            return source_buf;
        }
        else
        {
            read_offset += STDIN_READ_CHUNK_SIZE;
            source_buf = realloc(source_buf, read_offset + STDIN_READ_CHUNK_SIZE + 1);
        }
    }
}

static char *read_source_file(const char *path)
{
    char *file_contents = NULL;
    FILE *f = fopen(path, "r");
    if (!f)
    {
        goto err;
    }

    // Get the file size.
    if (fseek(f, 0, SEEK_END))
    {
        goto err;
    }
    int fsize = ftell(f);
    if (fseek(f, 0, SEEK_SET))
    {
        goto err;
    }

    // Allocate storage space on the heap for the file.
    file_contents = calloc(fsize + 1, 1);
    if (!file_contents)
    {
        printf("Unable to allocate sufficient space for the file\n");
        return NULL;
    }

    // Read the entire file.
    if (fsize != fread(file_contents, 1, fsize, f))
        goto err;
    fclose(f);

    return file_contents;
err:
    if (file_contents)
        free(file_contents);
    printf("Unable to read source file:\n%s\n", strerror(errno));
    return NULL;
}

static char *read_source(const char *path)
{
    if (*path == '-')
    {
        return read_source_stdin();
    }
    else
    {
        return read_source_file(path);
    }
}

static AccCompiler *compiler_init(const char *path)
{
    char *src = read_source(path);
    if (!src)
        return NULL;

    AccCompiler *compiler = calloc(1, sizeof(AccCompiler));
    compiler->source = src;
    compiler->error_reporter = Error_init();
    compiler->scanner = Scanner_init(src, compiler->error_reporter);
    compiler->parser = Parser_init(compiler->scanner, compiler->error_reporter);

    return compiler;
}

static DeclAstNode *compiler_parse(AccCompiler *compiler)
{
    return Parser_translation_unit(compiler->parser);
}

static void compiler_analysis(AccCompiler *compiler, DeclAstNode *ast_root)
{
    SymbolTable *global = symbol_table_create(NULL);
    analysis_ast_walk(compiler->error_reporter, ast_root, NULL, NULL, global);
}

static void print_error_json(ErrorType type, int line, int pos, char *msg)
{
    printf("\n    {\"error_type\":");
    switch (type)
    {
    case SCANNER:
        printf("\"SCANNER\"");
        break;
    case PARSER:
        printf("\"PARSER\"");
        break;
    case ANALYSIS:
        printf("\"ANALYSIS\"");
        break;
    }
    printf(", \"line_number\": %d", line);
    printf(", \"message\": \"%s\"}", msg);
}

static void print_error_commandline(ErrorType type, const char *line, int line_number,
                                    int pos, char *msg)
{
    int line_len = strchr(line, '\n') - line;
    char *line_cpy = malloc(line_len + 1);
    strncpy(line_cpy, line, line_len);
    line_cpy[line_len] = '\0';

    printf("\nError occurred on line %d ", line_number);
    switch (type)
    {
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
    printf("\nError: %s\n", msg);
    printf(" > %s\n", line_cpy);

    // Print out a '^' below the error.
    for (int i = 0; i < pos + 3; i++)
        printf(" ");
    printf("^\n");
}

static void compiler_print_errors(AccCompiler *compiler, _Bool json)
{
    int errors = 0;
    ErrorType type;
    int line_number;
    int line_position;
    char *msg;

    if (json)
    {
        printf("{\n  \"errors\":\n  [");
    }

    for (;; errors++)
    {
        if (!Error_get_errors(compiler->error_reporter, &type, &line_number,
                              &line_position, &msg, errors == 0))
            break;

        if (json)
        {
            if (errors)
                printf(",");
            print_error_json(type, line_number, line_position, msg);
        }
        else
        {
            const char *line = Scanner_get_line(compiler->scanner, line_number - 1);
            print_error_commandline(type, line, line_number, line_position, msg);
        }
    }

    if (!json)
    {
        printf("%d errors reported in total.\n", errors);
    }
    else
    {
        printf("\n  ]\n}\n");
    }
}

static void compiler_destroy(AccCompiler *compiler)
{
    Error_destroy(compiler->error_reporter);
    Parser_destroy(compiler->parser);
    Scanner_destroy(compiler->scanner);
    free(compiler->source);
}

int main(int argc, char **argv)
{
    CommandLineArgs args = {};
    AccCompiler *compiler;
    int err = 0;

    parse_cmd_args(argc, argv, &args);

    if (!(compiler = compiler_init(args.source_file)))
    {
        return 1;
    }

    // Generate the AST, from the source input.
    DeclAstNode *ast_root = compiler_parse(compiler);

    // Context-sensitive analysis on the AST.
    compiler_analysis(compiler, ast_root);

    // Check if errors occurred during scanning/parsing/analysis.
    // Abort if we cannot proceed.
    if (Error_has_errors(compiler->error_reporter))
    {
        compiler_print_errors(compiler, args.json);
        err = 1;
        goto tidyup;
    }

    if (args.check_only)
    {
        goto tidyup;
    }

    // Compiler to IR
    IrFunction *ir_program = Ir_generate(ast_root);
    Liveness_analysis(ir_program);

    // Register set.
    int * free_register_set = NULL;

    // Register allocation
    if (!args.omit_regalloc)
    {
        free_register_set = (int[]){4,5,6,7,8,9,10,11,12,-1};
        regalloc(ir_program, free_register_set);
    }

    if (args.ir_output)
    {
        if(strcmp(args.ir_output, "-") == 0)
        {
            Ir_to_str(stdout, ir_program, free_register_set);
        }
        else
        {
            FILE *ir_fh = fopen(args.ir_output, "w");
            Ir_to_str(ir_fh, ir_program, free_register_set);
            fclose(ir_fh);
        }
        return 0;
    }

    assembly_gen(stdout, ir_program);

tidyup:
    compiler_destroy(compiler);
    return err;
}
