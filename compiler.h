#ifndef COMPILER_H
#define COMPILER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================
// TOKEN TYPES
// ============================================
#define MAX_TOKENS 2000
#define MAX_SYMBOLS 200
#define MAX_TAC 2000
#define MAX_NAME 64
#define MAX_PARAMS 10
#define MAX_ARRAY_DIM 10

// Token categories
typedef enum {
    TOK_KEYWORD,
    TOK_IDENTIFIER,
    TOK_NUMBER,
    TOK_OPERATOR,
    TOK_PUNCTUATION,
    TOK_STRING,
    TOK_UNKNOWN
} TokenCategory;

// Token structure
typedef struct {
    char name[MAX_NAME];
    char value[MAX_NAME];
    TokenCategory category;
    int line;
} Token;

// ============================================
// SYMBOL TABLE
// ============================================
typedef struct {
    char name[MAX_NAME];
    char type[MAX_NAME];
    char scope[MAX_NAME];
    int line;
    int initialized;
    int is_array;
    int array_size;
    int is_pointer;
    int is_function;
    int param_count;
    char param_types[MAX_PARAMS][MAX_NAME];
} Symbol;

// ============================================
// THREE ADDRESS CODE (TAC)
// ============================================
typedef struct {
    char op[MAX_NAME];
    char arg1[MAX_NAME];
    char arg2[MAX_NAME];
    char result[MAX_NAME];
} TAC;

// ============================================
// AST NODE
// ============================================
typedef struct ASTNode {
    char type[MAX_NAME];
    char value[MAX_NAME];
    struct ASTNode *left;
    struct ASTNode *right;
    struct ASTNode *extra;
    struct ASTNode *extra2;
} ASTNode;

// ============================================
// GLOBAL ARRAYS
// ============================================
extern Token token_table[MAX_TOKENS];
extern int token_count;

extern Symbol symbol_table[MAX_SYMBOLS];
extern int symbol_count;

extern TAC tac_list[MAX_TAC];
extern int tac_count;

extern int error_count;
extern int temp_count;
extern int label_count;

// ============================================
// FUNCTION DECLARATIONS
// ============================================
const char* category_name(TokenCategory c);
void add_token(const char* name, const char* value, TokenCategory cat, int line);
void print_token_table();

void add_symbol(const char* name, const char* type, const char* scope, int line);
void add_symbol_ex(const char* name, const char* type, const char* scope,
                   int line, int is_array, int array_size,
                   int is_pointer, int is_function, int param_count);
Symbol* find_symbol(const char* name);
void print_symbol_table();

void add_tac(const char* op, const char* arg1, const char* arg2, const char* result);
void print_tac();
void optimize_tac();
void print_optimized_tac();

ASTNode* make_node(const char* type, const char* value, ASTNode* left, ASTNode* right);
void print_ast(ASTNode* node, int depth);

void generate_assembly();

char* new_temp();
char* new_label();

#endif