%{
  #include "compiler.h"
  #include <stdio.h>
  #include <stdlib.h>
  #include <string.h>

  extern int yylex();
  extern int yyline;
  void yyerror(const char* msg);

  char current_scope[MAX_NAME] = "global";
  ASTNode* ast_root = NULL;
%}

%union {
    char*            str;
    struct ASTNode*  node;
}

%token <str> ID NUMBER STRING_LIT
%token INT FLOAT CHAR VOID
%token IF ELSE WHILE FOR RETURN PRINTF
%token PLUS MINUS MUL DIV
%token ASSIGN EQ NEQ LT GT LEQ GEQ AND OR NOT
%token SEMI COMMA LPAREN RPAREN LBRACE RBRACE

%type <node> program decl_list decl
%type <node> func_decl func_body stmt_list stmt
%type <node> var_decl assign_stmt if_stmt while_stmt
%type <node> return_stmt printf_stmt expr term factor
%type <str>  type_spec

%right ASSIGN
%left  OR
%left  AND
%left  EQ NEQ
%left  LT GT LEQ GEQ
%left  PLUS MINUS
%left  MUL DIV
%right NOT

%%

program
    : decl_list
      {
          ast_root = make_node("PROGRAM", "root", $1, NULL);
          $$ = ast_root;
      }
    ;

decl_list
    : decl_list decl
      { $$ = make_node("DECL_LIST", "", $1, $2); }
    | decl
      { $$ = $1; }
    ;

decl
    : func_decl { $$ = $1; }
    | var_decl  { $$ = $1; }
    ;

type_spec
    : INT   { $$ = "int";   }
    | FLOAT { $$ = "float"; }
    | CHAR  { $$ = "char";  }
    | VOID  { $$ = "void";  }
    ;

var_decl
    : type_spec ID SEMI
      {
          Symbol* s = find_symbol($2);
          if(s != NULL && strcmp(s->scope, current_scope)==0) {
              printf("[SYN WARN] Line %d: '%s' already declared in scope '%s'\n",
                    yyline, $2, current_scope);
              error_count++;
          } else {
              add_symbol($2, $1, current_scope, yyline);
          }
          $$ = make_node("VAR_DECL", $2, NULL, NULL);
          add_tac("DECL", $1, "", $2);
      }
    | type_spec ID ASSIGN expr SEMI
      {
          Symbol* s = find_symbol($2);
          if(s != NULL && strcmp(s->scope, current_scope)==0) {
              printf("[SYN WARN] Line %d: '%s' already declared\n", yyline, $2);
              error_count++;
          } else {
              add_symbol($2, $1, current_scope, yyline);
              Symbol* ns = find_symbol($2);
              if(ns) ns->initialized = 1;
          }
          add_tac("=", $4->value, "", $2);
          $$ = make_node("VAR_INIT", $2, $4, NULL);
      }
    ;

func_decl
    : type_spec ID LPAREN RPAREN
      {
          strcpy(current_scope, $2);
          add_symbol($2, $1, "global", yyline);
          add_tac("FUNC_START", $2, "", "");
      }
      func_body
      {
          $$ = make_node("FUNC_DECL", $2, $6, NULL);
          add_tac("FUNC_END", $2, "", "");
          strcpy(current_scope, "global");
      }
    | type_spec ID LPAREN type_spec ID RPAREN
      {
          strcpy(current_scope, $2);
          add_symbol($2, $1, "global", yyline);
          add_symbol($5, $4, current_scope, yyline);
          add_tac("FUNC_START", $2, "", "");
      }
      func_body
      {
          $$ = make_node("FUNC_DECL", $2, $8, NULL);
          add_tac("FUNC_END", $2, "", "");
          strcpy(current_scope, "global");
      }
    ;

func_body
    : LBRACE stmt_list RBRACE
      { $$ = $2; }
    | LBRACE RBRACE
      { $$ = make_node("EMPTY_BODY", "", NULL, NULL); }
    ;

stmt_list
    : stmt_list stmt
      { $$ = make_node("STMT_LIST", "", $1, $2); }
    | stmt
      { $$ = $1; }
    ;

stmt
    : var_decl    { $$ = $1; }
    | assign_stmt { $$ = $1; }
    | if_stmt     { $$ = $1; }
    | while_stmt  { $$ = $1; }
    | return_stmt { $$ = $1; }
    | printf_stmt { $$ = $1; }
    | error SEMI
      {
          printf("[SYN ERROR] Line %d: Invalid statement\n", yyline);
          error_count++;
          $$ = make_node("ERROR_STMT", "", NULL, NULL);
          yyerrok;
      }
    ;

assign_stmt
    : ID ASSIGN expr SEMI
      {
          Symbol* s = find_symbol($1);
          if(s == NULL) {
              printf("[SYN WARN] Line %d: '%s' used before declaration\n",
                    yyline, $1);
              error_count++;
              add_symbol($1, "unknown", current_scope, yyline);
          } else {
              s->initialized = 1;
          }
          add_tac("=", $3->value, "", $1);
          $$ = make_node("ASSIGN", $1, $3, NULL);
      }
    ;

if_stmt
    : IF LPAREN expr RPAREN LBRACE stmt_list RBRACE
      {
          char lbl_end[MAX_NAME];
          strcpy(lbl_end, new_label());
          add_tac("IF_FALSE", $3->value, "", lbl_end);
          /* stmt_list এর TAC ইতিমধ্যে generate হয়েছে parser এ */
          add_tac("LABEL", lbl_end, "", "");
          $$ = make_node("IF", "", $3, $6);
      }
    | IF LPAREN expr RPAREN LBRACE stmt_list RBRACE
      ELSE LBRACE stmt_list RBRACE
      {
          char lbl_else[MAX_NAME];
          char lbl_end[MAX_NAME];
          strcpy(lbl_else, new_label());
          strcpy(lbl_end,  new_label());
          add_tac("IF_FALSE", $3->value, "", lbl_else);
          add_tac("GOTO",     "",         "", lbl_end);
          add_tac("LABEL",    lbl_else,   "", "");
          add_tac("LABEL",    lbl_end,    "", "");
          $$ = make_node("IF_ELSE", "", $3,
                        make_node("BRANCHES", "", $6, $10));
      }
    ;

while_stmt
    : WHILE LPAREN expr RPAREN LBRACE stmt_list RBRACE
      {
          char lbl_start[MAX_NAME];
          char lbl_end[MAX_NAME];
          strcpy(lbl_start, new_label());
          strcpy(lbl_end,   new_label());
          add_tac("LABEL",    lbl_start,  "", "");
          add_tac("IF_FALSE", $3->value,  "", lbl_end);
          add_tac("GOTO",     "",         "", lbl_start);
          add_tac("LABEL",    lbl_end,    "", "");
          $$ = make_node("WHILE", "", $3, $6);
      }
    ;

return_stmt
    : RETURN expr SEMI
      {
          add_tac("RETURN", $2->value, "", "");
          $$ = make_node("RETURN", $2->value, $2, NULL);
      }
    | RETURN SEMI
      {
          add_tac("RETURN", "", "", "");
          $$ = make_node("RETURN", "", NULL, NULL);
      }
    ;

printf_stmt
    : PRINTF LPAREN STRING_LIT RPAREN SEMI
      {
          add_tac("PRINT", $3, "", "");
          $$ = make_node("PRINTF", $3, NULL, NULL);
      }
    | PRINTF LPAREN STRING_LIT COMMA expr RPAREN SEMI
      {
          add_tac("PRINT", $3, $5->value, "");
          $$ = make_node("PRINTF", $3, $5, NULL);
      }
    ;

expr
    : expr PLUS term
      {
          char* t = new_temp();
          add_tac("+", $1->value, $3->value, t);
          $$ = make_node("ADD", t, $1, $3);
          strcpy($$->value, t);
      }
    | expr MINUS term
      {
          char* t = new_temp();
          add_tac("-", $1->value, $3->value, t);
          $$ = make_node("SUB", t, $1, $3);
          strcpy($$->value, t);
      }
    | expr LT term
      {
          char* t = new_temp();
          add_tac("<", $1->value, $3->value, t);
          $$ = make_node("LT", t, $1, $3);
          strcpy($$->value, t);
      }
    | expr GT term
      {
          char* t = new_temp();
          add_tac(">", $1->value, $3->value, t);
          $$ = make_node("GT", t, $1, $3);
          strcpy($$->value, t);
      }
    | expr LEQ term
      {
          char* t = new_temp();
          add_tac("<=", $1->value, $3->value, t);
          $$ = make_node("LEQ", t, $1, $3);
          strcpy($$->value, t);
      }
    | expr GEQ term
      {
          char* t = new_temp();
          add_tac(">=", $1->value, $3->value, t);
          $$ = make_node("GEQ", t, $1, $3);
          strcpy($$->value, t);
      }
    | expr EQ term
      {
          char* t = new_temp();
          add_tac("==", $1->value, $3->value, t);
          $$ = make_node("EQ", t, $1, $3);
          strcpy($$->value, t);
      }
    | expr NEQ term
      {
          char* t = new_temp();
          add_tac("!=", $1->value, $3->value, t);
          $$ = make_node("NEQ", t, $1, $3);
          strcpy($$->value, t);
      }
    | term
      { $$ = $1; }
    ;

term
    : term MUL factor
      {
          char* t = new_temp();
          add_tac("*", $1->value, $3->value, t);
          $$ = make_node("MUL", t, $1, $3);
          strcpy($$->value, t);
      }
    | term DIV factor
      {
          char* t = new_temp();
          add_tac("/", $1->value, $3->value, t);
          $$ = make_node("DIV", t, $1, $3);
          strcpy($$->value, t);
      }
    | factor
      { $$ = $1; }
    ;

factor
    : NUMBER
      { $$ = make_node("NUMBER", $1, NULL, NULL); }
    | ID
      {
          Symbol* s = find_symbol($1);
          if(s == NULL) {
              printf("[SYN WARN] Line %d: '%s' undeclared\n", yyline, $1);
              error_count++;
          }
          $$ = make_node("ID", $1, NULL, NULL);
      }
    | LPAREN expr RPAREN
      { $$ = $2; }
    | MINUS factor
      {
          char* t = new_temp();
          add_tac("NEG", $2->value, "", t);
          $$ = make_node("NEG", t, $2, NULL);
          strcpy($$->value, t);
      }
    ;

%%

void yyerror(const char* msg) {
    printf("[PARSE ERROR] Line %d: %s\n", yyline, msg);
    error_count++;
}