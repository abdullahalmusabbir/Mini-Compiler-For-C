#ifndef COMPILER_H
#define COMPILER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================
// TOKEN TYPES
// ============================================
#define MAX_TOKENS 1000
#define MAX_SYMBOLS 100
#define MAX_TAC 1000
#define MAX_NAME 64

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
    int  line;
    int  initialized;
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
    struct ASTNode *extra; // for 3rd child (if condition body)
} ASTNode;

// ============================================
// GLOBAL ARRAYS
// ============================================
extern Token    token_table[MAX_TOKENS];
extern int      token_count;

extern Symbol   symbol_table[MAX_SYMBOLS];
extern int      symbol_count;

extern TAC      tac_list[MAX_TAC];
extern int      tac_count;

extern int      error_count;
extern int      temp_count;
extern int      label_count;

// ============================================
// FUNCTION DECLARATIONS
// ============================================

// Token helpers
const char* category_name(TokenCategory c);
void add_token(const char* name, const char* value, TokenCategory cat, int line);
void print_token_table();

// Symbol table helpers
void add_symbol(const char* name, const char* type, const char* scope, int line);
Symbol* find_symbol(const char* name);
void print_symbol_table();

// TAC helpers
void add_tac(const char* op, const char* arg1, const char* arg2, const char* result);
void print_tac();
void optimize_tac();
void print_optimized_tac();

// AST helpers
ASTNode* make_node(const char* type, const char* value, ASTNode* left, ASTNode* right);
void print_ast(ASTNode* node, int depth);

// Assembly generation
void generate_assembly();

// Temp & label generators
char* new_temp();
char* new_label();

#endif