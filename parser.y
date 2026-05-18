%{
#include "compiler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int yylex();
extern int yyline;
void yyerror(const char* msg);

char current_scope[MAX_NAME] = "global";
char current_func[MAX_NAME]  = "";
ASTNode* ast_root = NULL;

static char param_type_buf[MAX_PARAMS][MAX_NAME];
static int  param_count_buf = 0;

static char arg_buf[MAX_PARAMS][MAX_NAME];
static int  arg_count_buf = 0;
%}

%union {
    char*           str;
    struct ASTNode* node;
}

%token <str> ID NUMBER STRING_LIT
%token INT FLOAT CHAR VOID
%token IF ELSE WHILE FOR RETURN PRINTF
%token STRUCT SWITCH CASE BREAK DEFAULT
%token PLUS MINUS MUL DIV
%token ASSIGN EQ NEQ LT GT LEQ GEQ AND OR NOT
%token AMP ARROW INC DEC
%token SEMI COMMA LPAREN RPAREN LBRACE RBRACE
%token LBRACKET RBRACKET DOT COLON

%type <node> program decl_list decl
%type <node> func_decl func_body stmt_list stmt
%type <node> var_decl assign_stmt if_stmt while_stmt for_stmt
%type <node> return_stmt printf_stmt expr term factor
%type <node> switch_stmt case_list case_item
%type <node> struct_decl
%type <node> func_call_stmt
%type <str>  type_spec

%right ASSIGN
%left  OR
%left  AND
%left  EQ NEQ
%left  LT GT LEQ GEQ
%left  PLUS MINUS
%left  MUL DIV
%right NOT AMP

%%

/* ================================================
   TOP LEVEL
   ================================================ */
program
    : decl_list
    {
        ast_root = make_node("PROGRAM","root",$1,NULL);
        $$ = ast_root;
    }
    ;

decl_list
    : decl_list decl  { $$ = make_node("DECL_LIST","",$1,$2); }
    | decl            { $$ = $1; }
    ;

decl
    : func_decl   { $$ = $1; }
    | var_decl    { $$ = $1; }
    | struct_decl { $$ = $1; }
    ;

/* ================================================
   TYPE SPECIFIER
   ================================================ */
type_spec
    : INT   { $$ = "int";   }
    | FLOAT { $$ = "float"; }
    | CHAR  { $$ = "char";  }
    | VOID  { $$ = "void";  }
    ;

/* ================================================
   STRUCT DECLARATION
   ================================================ */
struct_decl
    : STRUCT ID LBRACE stmt_list RBRACE SEMI
    {
        add_symbol_ex($2,"struct","global",yyline,0,0,0,0,0);
        add_tac("STRUCT_DEF",$2,"","");
        $$ = make_node("STRUCT_DECL",$2,$4,NULL);
    }
    ;

/* ================================================
   VARIABLE DECLARATION
   ================================================ */
var_decl
    /* int x; */
    : type_spec ID SEMI
    {
        if(find_symbol($2)==NULL)
            add_symbol_ex($2,$1,current_scope,yyline,0,0,0,0,0);
        else {
            printf("[WARN] Line %d: '%s' redeclared\n",yyline,$2);
            error_count++;
        }
        add_tac("DECL",$1,"",$2);
        $$ = make_node("VAR_DECL",$2,NULL,NULL);
    }
    /* int x = expr; */
    | type_spec ID ASSIGN expr SEMI
    {
        Symbol* s = find_symbol($2);
        if(s==NULL){
            add_symbol_ex($2,$1,current_scope,yyline,0,0,0,0,0);
            s = find_symbol($2);
            if(s) s->initialized = 1;
        } else {
            printf("[WARN] Line %d: '%s' redeclared\n",yyline,$2);
            error_count++;
        }
        add_tac("=",$4->value,"",$2);
        $$ = make_node("VAR_INIT",$2,$4,NULL);
    }
    /* int arr[10]; */
    | type_spec ID LBRACKET NUMBER RBRACKET SEMI
    {
        int sz = atoi($4);
        if(find_symbol($2)==NULL)
            add_symbol_ex($2,$1,current_scope,yyline,1,sz,0,0,0);
        else {
            printf("[WARN] Line %d: '%s' redeclared\n",yyline,$2);
            error_count++;
        }
        char szbuf[32];
        sprintf(szbuf,"%d",sz);
        add_tac("ARRAY_DECL",$1,szbuf,$2);
        $$ = make_node("ARRAY_DECL",$2,NULL,NULL);
    }
    /* int *ptr; */
    | type_spec MUL ID SEMI
    {
        if(find_symbol($3)==NULL)
            add_symbol_ex($3,$1,current_scope,yyline,0,0,1,0,0);
        else {
            printf("[WARN] Line %d: '%s' redeclared\n",yyline,$3);
            error_count++;
        }
        add_tac("PTR_DECL",$1,"",$3);
        $$ = make_node("PTR_DECL",$3,NULL,NULL);
    }
    ;

/* ================================================
   FUNCTION DECLARATION
   ================================================ */
func_decl
    /* type func() { } */
    : type_spec ID LPAREN RPAREN
    {
        strcpy(current_scope,$2);
        strcpy(current_func,$2);
        add_symbol_ex($2,$1,"global",yyline,0,0,0,1,0);
        add_tac("FUNC_START",$2,"","");
        param_count_buf = 0;
    }
    func_body
    {
        $$ = make_node("FUNC_DECL",$2,$6,NULL);
        add_tac("FUNC_END",$2,"","");
        strcpy(current_scope,"global");
        strcpy(current_func,"");
    }
    /* type func(params) { } */
    | type_spec ID LPAREN param_list RPAREN
    {
        strcpy(current_scope,$2);
        strcpy(current_func,$2);
        add_symbol_ex($2,$1,"global",yyline,0,0,0,1,param_count_buf);
        Symbol* fs = find_symbol($2);
        if(fs){
            fs->param_count = param_count_buf;
            for(int pi=0;pi<param_count_buf;pi++)
                strcpy(fs->param_types[pi], param_type_buf[pi]);
        }
        add_tac("FUNC_START",$2,"","");
    }
    func_body
    {
        $$ = make_node("FUNC_DECL",$2,$7,NULL);
        add_tac("FUNC_END",$2,"","");
        strcpy(current_scope,"global");
        strcpy(current_func,"");
        param_count_buf = 0;
    }
    ;

/* parameter list */
param_list
    : param_list COMMA param  { }
    | param                   { }
    ;

param
    : type_spec ID
    {
        if(param_count_buf < MAX_PARAMS){
            strcpy(param_type_buf[param_count_buf], $1);
            param_count_buf++;
        }
        add_symbol_ex($2,$1,current_scope,yyline,0,0,0,0,0);
        add_tac("PARAM",$1,"",$2);
    }
    | type_spec MUL ID
    {
        if(param_count_buf < MAX_PARAMS){
            char pt[MAX_NAME];
            sprintf(pt,"%s*",$1);
            strcpy(param_type_buf[param_count_buf], pt);
            param_count_buf++;
        }
        add_symbol_ex($3,$1,current_scope,yyline,0,0,1,0,0);
        add_tac("PARAM_PTR",$1,"",$3);
    }
    ;

func_body
    : LBRACE stmt_list RBRACE { $$ = $2; }
    | LBRACE RBRACE           { $$ = make_node("EMPTY_BODY","",NULL,NULL); }
    ;

/* ================================================
   STATEMENT LIST
   ================================================ */
stmt_list
    : stmt_list stmt { $$ = make_node("STMT_LIST","",$1,$2); }
    | stmt           { $$ = $1; }
    ;

stmt
    : var_decl       { $$ = $1; }
    | assign_stmt    { $$ = $1; }
    | if_stmt        { $$ = $1; }
    | while_stmt     { $$ = $1; }
    | for_stmt       { $$ = $1; }
    | switch_stmt    { $$ = $1; }
    | return_stmt    { $$ = $1; }
    | printf_stmt    { $$ = $1; }
    | func_call_stmt { $$ = $1; }
    | BREAK SEMI
    {
        add_tac("BREAK","","","");
        $$ = make_node("BREAK","",NULL,NULL);
    }
    | error SEMI
    {
        printf("[SYN ERROR] Line %d: Invalid statement\n",yyline);
        error_count++;
        $$ = make_node("ERROR_STMT","",NULL,NULL);
        yyerrok;
    }
    ;

/* ================================================
   FUNCTION CALL AS STATEMENT
   ================================================ */
func_call_stmt
    : ID LPAREN arg_list RPAREN SEMI
    {
        Symbol* s = find_symbol($1);
        if(s==NULL){
            printf("[WARN] Line %d: function '%s' undeclared\n",yyline,$1);
            error_count++;
        }
        int ai;
        for(ai=0; ai<arg_count_buf; ai++){
            add_tac("ARG","",arg_buf[ai],"");
        }
        char* t = new_temp();
        add_tac("CALL",$1,"",t);
        arg_count_buf = 0;
        $$ = make_node("FUNC_CALL",$1,NULL,NULL);
    }
    | ID LPAREN RPAREN SEMI
    {
        Symbol* s = find_symbol($1);
        if(s==NULL){
            printf("[WARN] Line %d: function '%s' undeclared\n",yyline,$1);
            error_count++;
        }
        char* t = new_temp();
        add_tac("CALL",$1,"",t);
        $$ = make_node("FUNC_CALL",$1,NULL,NULL);
    }
    ;

/* argument list */
arg_list
    : arg_list COMMA arg_item  { }
    | arg_item                 { }
    ;

arg_item
    : expr
    {
        if(arg_count_buf < MAX_PARAMS)
            strcpy(arg_buf[arg_count_buf++], $1->value);
    }
    ;

/* ================================================
   ASSIGN STATEMENT
   ================================================ */
assign_stmt
    /* x = expr; */
    : ID ASSIGN expr SEMI
    {
        Symbol* s = find_symbol($1);
        if(s==NULL){
            printf("[WARN] Line %d: '%s' undeclared\n",yyline,$1);
            error_count++;
            add_symbol_ex($1,"unknown",current_scope,yyline,0,0,0,0,0);
        } else {
            s->initialized = 1;
        }
        add_tac("=",$3->value,"",$1);
        $$ = make_node("ASSIGN",$1,$3,NULL);
    }
    /* arr[i] = expr; */
    | ID LBRACKET expr RBRACKET ASSIGN expr SEMI
    {
        add_tac("ARRAY_STORE",$6->value,$3->value,$1);
        $$ = make_node("ARRAY_ASSIGN",$1,$3,$6);
    }
    /* *ptr = expr; */
    | MUL ID ASSIGN expr SEMI
    {
        add_tac("PTR_STORE",$4->value,"",$2);
        $$ = make_node("PTR_ASSIGN",$2,$4,NULL);
    }
    /* x++; */
    | ID INC SEMI
    {
        add_tac("++",$1,"",$1);
        $$ = make_node("INC",$1,NULL,NULL);
    }
    /* x--; */
    | ID DEC SEMI
    {
        add_tac("--",$1,"",$1);
        $$ = make_node("DEC",$1,NULL,NULL);
    }
    ;

/* ================================================
   IF / IF-ELSE
   ================================================ */
if_stmt
    : IF LPAREN expr RPAREN LBRACE stmt_list RBRACE
    {
        char lbl_end[MAX_NAME];
        strcpy(lbl_end, new_label());
        add_tac("IF_FALSE",$3->value,"",lbl_end);
        add_tac("LABEL",lbl_end,"","");
        $$ = make_node("IF","",$3,$6);
    }
    | IF LPAREN expr RPAREN LBRACE stmt_list RBRACE
      ELSE LBRACE stmt_list RBRACE
    {
        char lbl_else[MAX_NAME];
        char lbl_end[MAX_NAME];
        strcpy(lbl_else, new_label());
        strcpy(lbl_end,  new_label());
        add_tac("IF_FALSE",$3->value,"",lbl_else);
        add_tac("GOTO","","",lbl_end);
        add_tac("LABEL",lbl_else,"","");
        add_tac("LABEL",lbl_end,"","");
        $$ = make_node("IF_ELSE","",$3,
             make_node("BRANCHES","",$6,$10));
    }
    ;

/* ================================================
   WHILE LOOP
   ================================================ */
while_stmt
    : WHILE LPAREN expr RPAREN LBRACE stmt_list RBRACE
    {
        char lbl_start[MAX_NAME];
        char lbl_end[MAX_NAME];
        strcpy(lbl_start, new_label());
        strcpy(lbl_end,   new_label());
        add_tac("LABEL",lbl_start,"","");
        add_tac("IF_FALSE",$3->value,"",lbl_end);
        add_tac("GOTO","","",lbl_start);
        add_tac("LABEL",lbl_end,"","");
        $$ = make_node("WHILE","",$3,$6);
    }
    ;

/* ================================================
   FOR LOOP
   ================================================ */
for_stmt
    : FOR LPAREN for_init SEMI expr SEMI for_update RPAREN
      LBRACE stmt_list RBRACE
    {
        char lbl_start[MAX_NAME];
        char lbl_end[MAX_NAME];
        strcpy(lbl_start, new_label());
        strcpy(lbl_end,   new_label());
        add_tac("FOR_LABEL",lbl_start,"","");
        add_tac("IF_FALSE",$5->value,"",lbl_end);
        add_tac("FOR_END",lbl_end,"","");
        $$ = make_node("FOR","",$5,$10);
    }
    ;

for_init
    : type_spec ID ASSIGN expr
    {
        if(find_symbol($2)==NULL)
            add_symbol_ex($2,$1,current_scope,yyline,0,0,0,0,0);
        add_tac("=",$4->value,"",$2);
    }
    | ID ASSIGN expr
    {
        add_tac("=",$3->value,"",$1);
    }
    | /* empty */ { }
    ;

for_update
    : ID ASSIGN expr  { add_tac("=",$3->value,"",$1); }
    | ID INC          { add_tac("++",$1,"",$1); }
    | ID DEC          { add_tac("--",$1,"",$1); }
    | /* empty */     { }
    ;

/* ================================================
   SWITCH - CASE
   ================================================ */
switch_stmt
    : SWITCH LPAREN expr RPAREN LBRACE case_list RBRACE
    {
        char lbl_end[MAX_NAME];
        strcpy(lbl_end, new_label());
        add_tac("SWITCH_END",lbl_end,"","");
        $$ = make_node("SWITCH","",$3,$6);
    }
    ;

case_list
    : case_list case_item { $$ = make_node("CASE_LIST","",$1,$2); }
    | case_item           { $$ = $1; }
    ;

case_item
    : CASE NUMBER COLON stmt_list
    {
        char lbl[MAX_NAME];
        strcpy(lbl, new_label());
        add_tac("CASE_LABEL",$2,"",lbl);
        $$ = make_node("CASE",$2,$4,NULL);
    }
    | DEFAULT COLON stmt_list
    {
        char lbl[MAX_NAME];
        strcpy(lbl, new_label());
        add_tac("DEFAULT_LABEL","","",lbl);
        $$ = make_node("DEFAULT","",$3,NULL);
    }
    ;

/* ================================================
   RETURN
   ================================================ */
return_stmt
    : RETURN expr SEMI
    {
        add_tac("RETURN",$2->value,"","");
        $$ = make_node("RETURN",$2->value,$2,NULL);
    }
    | RETURN SEMI
    {
        add_tac("RETURN","","","");
        $$ = make_node("RETURN","",NULL,NULL);
    }
    ;

/* ================================================
   PRINTF
   ================================================ */
printf_stmt
    : PRINTF LPAREN STRING_LIT RPAREN SEMI
    {
        add_tac("PRINT",$3,"","");
        $$ = make_node("PRINTF",$3,NULL,NULL);
    }
    | PRINTF LPAREN STRING_LIT COMMA expr RPAREN SEMI
    {
        add_tac("PRINT",$3,$5->value,"");
        $$ = make_node("PRINTF",$3,$5,NULL);
    }
    ;

/* ================================================
   EXPRESSIONS
   ================================================ */
expr
    : expr PLUS term
    {
        char* t = new_temp();
        add_tac("+",$1->value,$3->value,t);
        $$ = make_node("ADD",t,$1,$3);
        strcpy($$->value,t);
    }
    | expr MINUS term
    {
        char* t = new_temp();
        add_tac("-",$1->value,$3->value,t);
        $$ = make_node("SUB",t,$1,$3);
        strcpy($$->value,t);
    }
    /* && with short-circuit TAC */
    | expr AND expr
    {
        char* t        = new_temp();
        char* lbl_false = new_label();
        char* lbl_end   = new_label();
        /* copy label strings before new_label() overwrites buffer */
        char lf[MAX_NAME], le[MAX_NAME];
        strcpy(lf, lbl_false);
        strcpy(le, lbl_end);
        add_tac("IF_FALSE",$1->value,"",lf);
        add_tac("IF_FALSE",$3->value,"",lf);
        add_tac("=","1","",t);
        add_tac("GOTO","","",le);
        add_tac("LABEL",lf,"","");
        add_tac("=","0","",t);
        add_tac("LABEL",le,"","");
        $$ = make_node("AND",t,$1,$3);
        strcpy($$->value,t);
    }
    /* || with short-circuit TAC */
    | expr OR expr
    {
        char* t       = new_temp();
        char* lbl_true = new_label();
        char* lbl_end  = new_label();
        char lt[MAX_NAME], le[MAX_NAME];
        strcpy(lt, lbl_true);
        strcpy(le, lbl_end);
        add_tac("IF_TRUE",$1->value,"",lt);
        add_tac("IF_TRUE",$3->value,"",lt);
        add_tac("=","0","",t);
        add_tac("GOTO","","",le);
        add_tac("LABEL",lt,"","");
        add_tac("=","1","",t);
        add_tac("LABEL",le,"","");
        $$ = make_node("OR",t,$1,$3);
        strcpy($$->value,t);
    }
    | expr LT term
    {
        char* t = new_temp();
        add_tac("<",$1->value,$3->value,t);
        $$ = make_node("LT",t,$1,$3);
        strcpy($$->value,t);
    }
    | expr GT term
    {
        char* t = new_temp();
        add_tac(">",$1->value,$3->value,t);
        $$ = make_node("GT",t,$1,$3);
        strcpy($$->value,t);
    }
    | expr LEQ term
    {
        char* t = new_temp();
        add_tac("<=",$1->value,$3->value,t);
        $$ = make_node("LEQ",t,$1,$3);
        strcpy($$->value,t);
    }
    | expr GEQ term
    {
        char* t = new_temp();
        add_tac(">=",$1->value,$3->value,t);
        $$ = make_node("GEQ",t,$1,$3);
        strcpy($$->value,t);
    }
    | expr EQ term
    {
        char* t = new_temp();
        add_tac("==",$1->value,$3->value,t);
        $$ = make_node("EQ",t,$1,$3);
        strcpy($$->value,t);
    }
    | expr NEQ term
    {
        char* t = new_temp();
        add_tac("!=",$1->value,$3->value,t);
        $$ = make_node("NEQ",t,$1,$3);
        strcpy($$->value,t);
    }
    | term { $$ = $1; }
    ;

term
    : term MUL factor
    {
        char* t = new_temp();
        add_tac("*",$1->value,$3->value,t);
        $$ = make_node("MUL",t,$1,$3);
        strcpy($$->value,t);
    }
    | term DIV factor
    {
        char* t = new_temp();
        add_tac("/",$1->value,$3->value,t);
        $$ = make_node("DIV",t,$1,$3);
        strcpy($$->value,t);
    }
    | factor { $$ = $1; }
    ;

factor
    : NUMBER
    {
        $$ = make_node("NUMBER",$1,NULL,NULL);
    }
    /* array access: arr[i] */
    | ID LBRACKET expr RBRACKET
    {
        char* t = new_temp();
        add_tac("ARRAY_LOAD",$1,$3->value,t);
        $$ = make_node("ARRAY_ACCESS",t,NULL,NULL);
        strcpy($$->value,t);
    }
    /* function call in expr: foo(args) */
    | ID LPAREN arg_list RPAREN
    {
        int ai;
        for(ai=0; ai<arg_count_buf; ai++)
            add_tac("ARG","",arg_buf[ai],"");
        char* t = new_temp();
        add_tac("CALL",$1,"",t);
        arg_count_buf = 0;
        $$ = make_node("FUNC_CALL_EXPR",t,NULL,NULL);
        strcpy($$->value,t);
    }
    /* function call no args */
    | ID LPAREN RPAREN
    {
        char* t = new_temp();
        add_tac("CALL",$1,"",t);
        $$ = make_node("FUNC_CALL_EXPR",t,NULL,NULL);
        strcpy($$->value,t);
    }
    /* normal identifier - must come AFTER array & call rules */
    | ID
    {
        if(find_symbol($1)==NULL){
            printf("[WARN] Line %d: '%s' undeclared\n",yyline,$1);
            error_count++;
        }
        $$ = make_node("ID",$1,NULL,NULL);
    }
    /* pointer dereference: *ptr */
    | MUL ID
    {
        char* t = new_temp();
        add_tac("PTR_LOAD",$2,"",t);
        $$ = make_node("PTR_DEREF",t,NULL,NULL);
        strcpy($$->value,t);
    }
    /* address-of: &x */
    | AMP ID
    {
        char* t = new_temp();
        add_tac("ADDR",$2,"",t);
        $$ = make_node("ADDR_OF",t,NULL,NULL);
        strcpy($$->value,t);
    }
    | LPAREN expr RPAREN
    {
        $$ = $2;
    }
    | MINUS factor
    {
        char* t = new_temp();
        add_tac("NEG",$2->value,"",t);
        $$ = make_node("NEG",t,$2,NULL);
        strcpy($$->value,t);
    }
    | NOT factor
    {
        char* t = new_temp();
        add_tac("NOT",$2->value,"",t);
        $$ = make_node("NOT",t,$2,NULL);
        strcpy($$->value,t);
    }
    | STRING_LIT
    {
        $$ = make_node("STRING",$1,NULL,NULL);
    }
    ;

%%

void yyerror(const char* msg){
    printf("[PARSE ERROR] Line %d: %s\n",yyline,msg);
    error_count++;
}