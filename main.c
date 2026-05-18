#include "compiler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================
   GLOBALS
   ============================================ */
Token   token_table[MAX_TOKENS];
int     token_count = 0;

Symbol  symbol_table[MAX_SYMBOLS];
int     symbol_count = 0;

TAC     tac_list[MAX_TAC];
int     tac_count = 0;

int     error_count = 0;
int     temp_count  = 0;
int     label_count = 0;

static char temp_buf [MAX_NAME];
static char label_buf[MAX_NAME];

extern ASTNode* ast_root;
extern int yyparse();
extern FILE* yyin;

/* ============================================
   BASIC HELPERS
   ============================================ */
char* new_temp(){
    sprintf(temp_buf,"t%d",temp_count++);
    return temp_buf;
}
char* new_label(){
    sprintf(label_buf,"L%d",label_count++);
    return label_buf;
}

const char* category_name(TokenCategory c){
    switch(c){
        case TOK_KEYWORD:     return "KEYWORD";
        case TOK_IDENTIFIER:  return "IDENTIFIER";
        case TOK_NUMBER:      return "NUMBER";
        case TOK_OPERATOR:    return "OPERATOR";
        case TOK_PUNCTUATION: return "PUNCTUATION";
        case TOK_STRING:      return "STRING";
        default:              return "UNKNOWN";
    }
}

void add_token(const char* name,const char* value,
               TokenCategory cat,int line){
    if(token_count>=MAX_TOKENS) return;
    strncpy(token_table[token_count].name, name, MAX_NAME-1);
    strncpy(token_table[token_count].value,value,MAX_NAME-1);
    token_table[token_count].category = cat;
    token_table[token_count].line     = line;
    token_count++;
}

void add_symbol(const char* name,const char* type,
                const char* scope,int line){
    add_symbol_ex(name,type,scope,line,0,0,0,0,0);
}

void add_symbol_ex(const char* name,const char* type,
                   const char* scope,int line,
                   int is_array,int array_size,
                   int is_pointer,int is_function,int param_count){
    if(symbol_count>=MAX_SYMBOLS) return;
    strncpy(symbol_table[symbol_count].name,  name,  MAX_NAME-1);
    strncpy(symbol_table[symbol_count].type,  type,  MAX_NAME-1);
    strncpy(symbol_table[symbol_count].scope, scope, MAX_NAME-1);
    symbol_table[symbol_count].line        = line;
    symbol_table[symbol_count].initialized = 0;
    symbol_table[symbol_count].is_array    = is_array;
    symbol_table[symbol_count].array_size  = array_size;
    symbol_table[symbol_count].is_pointer  = is_pointer;
    symbol_table[symbol_count].is_function = is_function;
    symbol_table[symbol_count].param_count = param_count;
    symbol_count++;
}

Symbol* find_symbol(const char* name){
    for(int i=0;i<symbol_count;i++)
        if(strcmp(symbol_table[i].name,name)==0)
            return &symbol_table[i];
    return NULL;
}

void add_tac(const char* op,const char* arg1,
             const char* arg2,const char* result){
    if(tac_count>=MAX_TAC) return;
    strncpy(tac_list[tac_count].op,    op,    MAX_NAME-1);
    strncpy(tac_list[tac_count].arg1,  arg1,  MAX_NAME-1);
    strncpy(tac_list[tac_count].arg2,  arg2,  MAX_NAME-1);
    strncpy(tac_list[tac_count].result,result,MAX_NAME-1);
    tac_count++;
}

ASTNode* make_node(const char* type,const char* value,
                   ASTNode* left,ASTNode* right){
    ASTNode* n = (ASTNode*)malloc(sizeof(ASTNode));
    strncpy(n->type, type, MAX_NAME-1);
    strncpy(n->value,value,MAX_NAME-1);
    n->left  = left;
    n->right = right;
    n->extra = NULL;
    n->extra2= NULL;
    return n;
}

/* ============================================
   HELPERS
   ============================================ */
static int is_number(const char* s){
    if(!s||strlen(s)==0) return 0;
    int i=0;
    if(s[0]=='-') i=1;
    for(;s[i];i++)
        if(s[i]!='.'&&(s[i]<'0'||s[i]>'9')) return 0;
    return 1;
}

static int is_function_sym(const char* name){
    Symbol* s = find_symbol(name);
    return (s && s->is_function);
}

static void print_header(const char* title){
    printf("\n");
    printf("==================================================================\n");
    printf("  %s\n",title);
    printf("==================================================================\n");
}
static void print_line(){
    printf("------------------------------------------------------------------\n");
}

/* ============================================
   STEP 1: TOKEN TABLE
   ============================================ */
void print_token_table(){
    print_header("STEP 1: LEXICAL ANALYSIS - TOKEN TABLE");
    printf("  %-5s %-15s %-22s %-15s %s\n",
           "No.","TOKEN NAME","TOKEN VALUE","CATEGORY","LINE");
    print_line();
    for(int i=0;i<token_count;i++){
        printf("  %-5d %-15s %-22s %-15s %d\n",
               i+1,
               token_table[i].name,
               token_table[i].value,
               category_name(token_table[i].category),
               token_table[i].line);
    }
    print_line();
    int cnt[7]={0};
    for(int i=0;i<token_count;i++) cnt[(int)token_table[i].category]++;
    printf("\n  Token Category Summary:\n");
    printf("  KEYWORD      : %d\n",cnt[TOK_KEYWORD]);
    printf("  IDENTIFIER   : %d\n",cnt[TOK_IDENTIFIER]);
    printf("  NUMBER       : %d\n",cnt[TOK_NUMBER]);
    printf("  OPERATOR     : %d\n",cnt[TOK_OPERATOR]);
    printf("  PUNCTUATION  : %d\n",cnt[TOK_PUNCTUATION]);
    printf("  STRING       : %d\n",cnt[TOK_STRING]);
    printf("  UNKNOWN      : %d\n",cnt[TOK_UNKNOWN]);
    printf("  -----------------\n");
    printf("  TOTAL        : %d\n",token_count);
}

/* ============================================
   STEP 2: SYMBOL TABLE
   ============================================ */
void print_symbol_table(){
    print_header("STEP 2: SYMBOL TABLE");
    printf("  %-18s %-8s %-10s %-10s %-6s %-5s %-5s %s\n",
           "NAME","TYPE","KIND","SCOPE","LINE","INIT","SIZE","PARAMS");
    print_line();
    for(int i=0;i<symbol_count;i++){
        Symbol* s=&symbol_table[i];
        const char* kind;
        if(s->is_function) kind="Function";
        else if(s->is_array) kind="Array";
        else if(s->is_pointer) kind="Pointer";
        else kind="Variable";

        printf("  %-18s %-8s %-10s %-10s %-6d %-5s",
               s->name, s->type, kind, s->scope,
               s->line, s->initialized?"YES":"NO");
        if(s->is_array)    printf(" %-5d",s->array_size);
        else               printf(" %-5s","-");
        if(s->is_function) printf(" %d",s->param_count);
        else               printf(" -");
        printf("\n");
    }
    print_line();
    int fc=0,vc=0,ac=0,pc=0;
    for(int i=0;i<symbol_count;i++){
        if(symbol_table[i].is_function) fc++;
        else if(symbol_table[i].is_array) ac++;
        else if(symbol_table[i].is_pointer) pc++;
        else vc++;
    }
    printf("  Total: %d  |  Functions:%d  Variables:%d  Arrays:%d  Pointers:%d\n",
           symbol_count,fc,vc,ac,pc);
}

/* ============================================
   STEP 3: SYNTAX & SEMANTIC ANALYSIS
   ============================================ */
void print_token_stream(){
    print_header("STEP 3: SYNTAX & SEMANTIC ANALYSIS");
    printf("  Input File: test.c\n\n");
    print_line();
    int syn_err=0, sem_err=0;
    for(int i=0;i<token_count;i++){
        printf("  Token %-3d : %-14s | %-22s | Line: %d\n",
               i+1,
               category_name(token_table[i].category),
               token_table[i].value,
               token_table[i].line);
        if(token_table[i].category==TOK_IDENTIFIER){
            if(find_symbol(token_table[i].value)==NULL){
                printf("  [!] SEMANTIC: '%s' undeclared at line %d\n",
                       token_table[i].value, token_table[i].line);
                sem_err++;
            }
        }
        if(token_table[i].category==TOK_UNKNOWN){
            printf("  [!] SYNTAX: unknown token '%s' at line %d\n",
                   token_table[i].value, token_table[i].line);
            syn_err++;
        }
    }
    print_line();
    printf("  Syntax Errors : %d\n",syn_err);
    printf("  Semantic Errors: %d\n",sem_err);
    if(syn_err==0&&sem_err==0)
        printf("  [OK] ANALYSIS PASSED\n");
    else
        printf("  [FAIL] ANALYSIS FAILED\n");
    print_line();
}

/* ============================================
   STEP 4: AST
   ============================================ */
static int should_skip(ASTNode* n){
    if(!n) return 1;
    return (strcmp(n->type,"PROGRAM")==0  ||
            strcmp(n->type,"DECL_LIST")==0||
            strcmp(n->type,"STMT_LIST")==0||
            strcmp(n->type,"CASE_LIST")==0||
            strcmp(n->type,"BRANCHES")==0 ||
            strcmp(n->type,"VAR_DECL")==0);
}

static void node_label(ASTNode* n,char* buf){
    buf[0]='\0';
    if(!n) return;
    if     (strcmp(n->type,"FUNC_DECL")==0)  sprintf(buf,"Function: %s",n->value);
    else if(strcmp(n->type,"VAR_INIT")==0)   strcpy(buf,"=");
    else if(strcmp(n->type,"ASSIGN")==0)     sprintf(buf,"= (%s)",n->value);
    else if(strcmp(n->type,"ARRAY_ASSIGN")==0) sprintf(buf,"arr[i]= (%s)",n->value);
    else if(strcmp(n->type,"PTR_ASSIGN")==0) sprintf(buf,"*ptr= (%s)",n->value);
    else if(strcmp(n->type,"ARRAY_DECL")==0) sprintf(buf,"array: %s[%d]",n->value,0);
    else if(strcmp(n->type,"PTR_DECL")==0)   sprintf(buf,"ptr: *%s",n->value);
    else if(strcmp(n->type,"STRUCT_DECL")==0)sprintf(buf,"struct: %s",n->value);
    else if(strcmp(n->type,"NUMBER")==0)     sprintf(buf,"NUM: %s",n->value);
    else if(strcmp(n->type,"ID")==0)         sprintf(buf,"ID: %s",n->value);
    else if(strcmp(n->type,"ARRAY_ACCESS")==0) sprintf(buf,"arr[]: %s",n->value);
    else if(strcmp(n->type,"PTR_DEREF")==0)  sprintf(buf,"*ptr: %s",n->value);
    else if(strcmp(n->type,"ADDR_OF")==0)    sprintf(buf,"&addr: %s",n->value);
    else if(strcmp(n->type,"FUNC_CALL")==0)  sprintf(buf,"call: %s()",n->value);
    else if(strcmp(n->type,"FUNC_CALL_EXPR")==0) sprintf(buf,"call_expr: %s",n->value);
    else if(strcmp(n->type,"ADD")==0)        strcpy(buf,"+");
    else if(strcmp(n->type,"SUB")==0)        strcpy(buf,"-");
    else if(strcmp(n->type,"MUL")==0)        strcpy(buf,"*");
    else if(strcmp(n->type,"DIV")==0)        strcpy(buf,"/");
    else if(strcmp(n->type,"AND")==0)        strcpy(buf,"&&");
    else if(strcmp(n->type,"OR")==0)         strcpy(buf,"||");
    else if(strcmp(n->type,"NOT")==0)        strcpy(buf,"!");
    else if(strcmp(n->type,"NEG")==0)        strcpy(buf,"NEG");
    else if(strcmp(n->type,"LT")==0)         strcpy(buf,"<");
    else if(strcmp(n->type,"GT")==0)         strcpy(buf,">");
    else if(strcmp(n->type,"LEQ")==0)        strcpy(buf,"<=");
    else if(strcmp(n->type,"GEQ")==0)        strcpy(buf,">=");
    else if(strcmp(n->type,"EQ")==0)         strcpy(buf,"==");
    else if(strcmp(n->type,"NEQ")==0)        strcpy(buf,"!=");
    else if(strcmp(n->type,"IF")==0)         strcpy(buf,"if");
    else if(strcmp(n->type,"IF_ELSE")==0)    strcpy(buf,"if-else");
    else if(strcmp(n->type,"WHILE")==0)      strcpy(buf,"while");
    else if(strcmp(n->type,"FOR")==0)        strcpy(buf,"for");
    else if(strcmp(n->type,"SWITCH")==0)     strcpy(buf,"switch");
    else if(strcmp(n->type,"CASE")==0)       sprintf(buf,"case %s",n->value);
    else if(strcmp(n->type,"DEFAULT")==0)    strcpy(buf,"default");
    else if(strcmp(n->type,"BREAK")==0)      strcpy(buf,"break");
    else if(strcmp(n->type,"RETURN")==0){
        if(strlen(n->value)>0) sprintf(buf,"return %s",n->value);
        else strcpy(buf,"return");
    }
    else if(strcmp(n->type,"PRINTF")==0)     sprintf(buf,"printf(%s)",n->value);
    else if(strcmp(n->type,"INC")==0)        sprintf(buf,"%s++",n->value);
    else if(strcmp(n->type,"DEC")==0)        sprintf(buf,"%s--",n->value);
    else if(strcmp(n->type,"EMPTY_BODY")==0) strcpy(buf,"(empty)");
    else if(strlen(n->value)>0) sprintf(buf,"%s: %s",n->type,n->value);
    else strcpy(buf,n->type);
}

static void collect_children(ASTNode* node,ASTNode** ch,int* cnt){
    *cnt=0;
    if(!node) return;
    ASTNode* direct[4];
    int dc=0;
    if(node->left)  direct[dc++]=node->left;
    if(node->right) direct[dc++]=node->right;
    if(node->extra) direct[dc++]=node->extra;
    if(node->extra2)direct[dc++]=node->extra2;
    for(int i=0;i<dc;i++){
        ASTNode* c=direct[i];
        if(!c) continue;
        if(should_skip(c)){
            ASTNode* q[256]; int qh=0,qt=0;
            q[qt++]=c;
            while(qh<qt){
                ASTNode* cur=q[qh++];
                if(!cur) continue;
                if(should_skip(cur)){
                    if(cur->left)  q[qt++]=cur->left;
                    if(cur->right) q[qt++]=cur->right;
                    if(cur->extra) q[qt++]=cur->extra;
                } else {
                    if(*cnt<64) ch[(*cnt)++]=cur;
                }
            }
        } else {
            if(*cnt<64) ch[(*cnt)++]=c;
        }
    }
}

static void print_tree(ASTNode* node, char* prefix, int is_last){
    if(!node) return;

    /* ASCII characters ব্যবহার করো Unicode এর বদলে */
    printf("%s%s", prefix, is_last ? "L-- " : "|-- ");

    char label[MAX_NAME*2];
    node_label(node, label);
    printf("%s\n", label);

    ASTNode* ch[64];
    int cnt = 0;
    collect_children(node, ch, &cnt);

    char new_pre[512];
    /* "|   " এর বদলে "|   " ASCII use করো */
    sprintf(new_pre, "%s%s", prefix, is_last ? "    " : "|   ");

    for(int i = 0; i < cnt; i++)
        print_tree(ch[i], new_pre, i == cnt-1);
}

void print_ast(ASTNode* node,int depth){
    (void)depth;
    if(!node) return;
    ASTNode* ch[64]; int cnt=0;
    collect_children(node,ch,&cnt);
    char empty[4]="";
    for(int i=0;i<cnt;i++)
        print_tree(ch[i],empty,i==cnt-1);
}

void print_ast_section(){
    print_header("STEP 4: ABSTRACT SYNTAX TREE");
    printf("\n");
    if(!ast_root){ printf("  [!] AST empty!\n"); return; }
    print_ast(ast_root,0);
    printf("\n");
    print_line();
    printf("  [OK] AST generated!\n");
    print_line();
}

/* ============================================
   STEP 5: TAC
   ============================================ */
typedef struct { char name[MAX_NAME]; int num; } IdMap;
static IdMap id_map[MAX_SYMBOLS];
static int   id_map_cnt=0;
static void reset_id_map(){ id_map_cnt=0; }
static int get_id_num(const char* name){
    for(int i=0;i<id_map_cnt;i++)
        if(strcmp(id_map[i].name,name)==0) return id_map[i].num;
    strncpy(id_map[id_map_cnt].name,name,MAX_NAME-1);
    id_map[id_map_cnt].num=id_map_cnt+1;
    return id_map[id_map_cnt++].num;
}

static void fmt_arg(const char* arg,char* out){
    out[0]='\0';
    if(!arg||strlen(arg)==0) return;
    if(arg[0]=='t'&&strlen(arg)>1&&arg[1]>='0'&&arg[1]<='9'){strcpy(out,arg);return;}
    if(arg[0]=='L'&&strlen(arg)>1&&arg[1]>='0'&&arg[1]<='9'){strcpy(out,arg);return;}
    if(is_number(arg)){sprintf(out,"num(%s)",arg);return;}
    int n=get_id_num(arg);
    sprintf(out,"id%d(%s)",n,arg);
}

static int is_meta_op(const char* op){
    return(strcmp(op,"FUNC_START")==0||strcmp(op,"FUNC_END")==0||
           strcmp(op,"LABEL")==0||strcmp(op,"DECL")==0||
           strcmp(op,"ARRAY_DECL")==0||strcmp(op,"PTR_DECL")==0||
           strcmp(op,"STRUCT_DEF")==0||strcmp(op,"PARAM")==0||
           strcmp(op,"PARAM_PTR")==0||strcmp(op,"NOP")==0||
           strcmp(op,"FOR_LABEL")==0||strcmp(op,"FOR_END")==0||
           strcmp(op,"CASE_LABEL")==0||strcmp(op,"DEFAULT_LABEL")==0||
           strcmp(op,"SWITCH_END")==0);
}

static void print_one_tac(TAC* t,int idx){
    if(strcmp(t->op,"NOP")==0) return;
    if(strcmp(t->op,"DECL")==0) return;
    if(strcmp(t->op,"ARRAY_DECL")==0) return;
    if(strcmp(t->op,"PTR_DECL")==0) return;
    if(strcmp(t->op,"STRUCT_DEF")==0) return;
    if(strcmp(t->op,"PARAM")==0){
        printf("  %3d: param %s %s\n",idx,t->arg1,t->result); return;
    }
    if(strcmp(t->op,"PARAM_PTR")==0){
        printf("  %3d: param %s* %s\n",idx,t->arg1,t->result); return;
    }

    char a1[MAX_NAME*2],a2[MAX_NAME*2],res[MAX_NAME*2];
    fmt_arg(t->arg1,a1);
    fmt_arg(t->arg2,a2);
    fmt_arg(t->result,res);

    if(strcmp(t->op,"FUNC_START")==0){
        printf("\n  >>> Function: %s <<<\n",t->arg1); return;
    }
    if(strcmp(t->op,"FUNC_END")==0){
        printf("  >>> End: %s <<<\n\n",t->arg1); return;
    }
    if(strcmp(t->op,"LABEL")==0||strcmp(t->op,"FOR_LABEL")==0||
       strcmp(t->op,"CASE_LABEL")==0||strcmp(t->op,"DEFAULT_LABEL")==0){
        printf("  %s:\n",t->arg1[0]?t->arg1:t->result); return;
    }
    if(strcmp(t->op,"SWITCH_END")==0||strcmp(t->op,"FOR_END")==0){
        printf("  %s:\n",t->result); return;
    }
    if(strcmp(t->op,"GOTO")==0){
        printf("  %3d: goto %s\n",idx,t->result); return;
    }
    if(strcmp(t->op,"IF_FALSE")==0){
        printf("  %3d: if_false %s goto %s\n",idx,a1,t->result); return;
    }
    if(strcmp(t->op,"IF_TRUE")==0){
        printf("  %3d: if_true %s goto %s\n",idx,a1,t->result); return;
    }
    if(strcmp(t->op,"RETURN")==0){
        if(strlen(t->arg1)>0) printf("  %3d: return %s\n",idx,a1);
        else printf("  %3d: return\n",idx);
        return;
    }
    if(strcmp(t->op,"PRINT")==0){
        if(strlen(t->arg2)>0)
            printf("  %3d: print %s , %s\n",idx,t->arg1,a2);
        else
            printf("  %3d: print %s\n",idx,t->arg1);
        return;
    }
    if(strcmp(t->op,"ARG")==0){
        printf("  %3d: arg %s\n",idx,a2); return;
    }
    if(strcmp(t->op,"CALL")==0){
        printf("  %3d: %s = call %s\n",idx,res,t->arg1); return;
    }
    if(strcmp(t->op,"ARRAY_LOAD")==0){
        printf("  %3d: %s = %s[%s]\n",idx,res,t->arg1,a2); return;
    }
    if(strcmp(t->op,"ARRAY_STORE")==0){
        printf("  %3d: %s[%s] = %s\n",idx,t->result,a2,a1); return;
    }
    if(strcmp(t->op,"PTR_LOAD")==0){
        printf("  %3d: %s = *%s\n",idx,res,t->arg1); return;
    }
    if(strcmp(t->op,"PTR_STORE")==0){
        printf("  %3d: *%s = %s\n",idx,t->result,a1); return;
    }
    if(strcmp(t->op,"ADDR")==0){
        printf("  %3d: %s = &%s\n",idx,res,t->arg1); return;
    }
    if(strcmp(t->op,"BREAK")==0){
        printf("  %3d: break\n",idx); return;
    }
    if(strcmp(t->op,"++")==0){
        printf("  %3d: %s = %s + 1\n",idx,res,a1); return;
    }
    if(strcmp(t->op,"--")==0){
        printf("  %3d: %s = %s - 1\n",idx,res,a1); return;
    }
    if(strcmp(t->op,"NOT")==0){
        printf("  %3d: %s = !%s\n",idx,res,a1); return;
    }
    if(strcmp(t->op,"NEG")==0){
        printf("  %3d: %s = -%s\n",idx,res,a1); return;
    }
    if(strcmp(t->op,"=")==0){
        printf("  %3d: %s = %s\n",idx,res,a1); return;
    }
    /* binary */
    printf("  %3d: %s = %s %s %s\n",idx,res,a1,t->op,a2);
}

static void print_tokenized(){
    reset_id_map();
    printf("  Tokenized form:\n  ");
    int first=1;
    for(int i=0;i<token_count;i++){
        Token* tk=&token_table[i];
        if(tk->category==TOK_PUNCTUATION&&
           (strcmp(tk->value,";")==0||strcmp(tk->value,"{")==0||
            strcmp(tk->value,"}")==0)) continue;
        if(!first) printf(", ");
        first=0;
        if(tk->category==TOK_IDENTIFIER){
            int n=get_id_num(tk->value);
            printf("<id%d,%s>",n,tk->value);
        } else if(tk->category==TOK_NUMBER){
            printf("<num,%s>",tk->value);
        } else if(tk->category==TOK_STRING){
            printf("<str,%s>",tk->value);
        } else {
            printf("<%s>",tk->value);
        }
    }
    printf("\n\n");
}

void print_tac(){
    print_header("STEP 5: THREE ADDRESS CODE (TAC)");
    reset_id_map();
    print_tokenized();
    printf("  Three Address Code:\n");
    print_line();
    reset_id_map();
    int idx=1;
    for(int i=0;i<tac_count;i++){
        print_one_tac(&tac_list[i],idx);
        if(!is_meta_op(tac_list[i].op)) idx++;
    }
    print_line();
    printf("  Total TAC instructions: %d\n",idx-1);
}

/* ============================================
   STEP 6: OPTIMIZATION
   ============================================ */
void optimize_tac(){
    /* Copy Propagation */
    for(int i=0;i<tac_count;i++){
        if(strcmp(tac_list[i].op,"=")==0&&
           tac_list[i].result[0]=='t'&&
           tac_list[i].result[1]>='0'){
            char* from=tac_list[i].result;
            char* to  =tac_list[i].arg1;
            for(int j=i+1;j<tac_count;j++){
                if(strcmp(tac_list[j].arg1,from)==0) strcpy(tac_list[j].arg1,to);
                if(strcmp(tac_list[j].arg2,from)==0) strcpy(tac_list[j].arg2,to);
                if(strcmp(tac_list[j].result,from)==0) break;
            }
            strcpy(tac_list[i].op,"NOP");
        }
    }
    /* Constant Folding */
    for(int i=0;i<tac_count;i++){
        TAC* t=&tac_list[i];
        if(!is_number(t->arg1)||!is_number(t->arg2)) continue;
        if(strlen(t->result)==0) continue;
        double n1=atof(t->arg1),n2=atof(t->arg2),res=0;
        int ok=1;
        if     (strcmp(t->op,"+")==0) res=n1+n2;
        else if(strcmp(t->op,"-")==0) res=n1-n2;
        else if(strcmp(t->op,"*")==0) res=n1*n2;
        else if(strcmp(t->op,"/")==0&&n2!=0) res=n1/n2;
        else ok=0;
        if(ok){
            char buf[MAX_NAME];
            if(res==(int)res) sprintf(buf,"%d",(int)res);
            else sprintf(buf,"%f",res);
            strcpy(t->op,"=");
            strcpy(t->arg1,buf);
            strcpy(t->arg2,"");
        }
    }
}

void print_optimized_tac(){
    print_header("STEP 6: CODE OPTIMIZATION - Optimized TAC");
    printf("  Optimizations: [1] Copy Propagation  [2] Constant Folding\n\n");
    printf("  Optimized Three Address Code:\n");
    print_line();
    reset_id_map();
    int count=0,idx=1;
    for(int i=0;i<tac_count;i++){
        if(strcmp(tac_list[i].op,"NOP")==0) continue;
        print_one_tac(&tac_list[i],idx);
        if(!is_meta_op(tac_list[i].op)){idx++;count++;}
    }
    print_line();
    printf("  Total after optimization: %d\n",count);
}

/* ============================================
   STEP 7: ASSEMBLY CODE GENERATION
   ============================================ */
void generate_assembly(){
    print_header("STEP 7: ASSEMBLY CODE GENERATION (x86 NASM style)");

    /* section .data */
    printf("\nsection .data\n");
    for(int i=0;i<symbol_count;i++){
        Symbol* s=&symbol_table[i];
        if(s->is_function) continue;
        if(s->is_array){
            if(strcmp(s->type,"int")==0)
                printf("  %-16s times %d dd 0\n",s->name,s->array_size);
            else if(strcmp(s->type,"float")==0)
                printf("  %-16s times %d dq 0.0\n",s->name,s->array_size);
            else if(strcmp(s->type,"char")==0)
                printf("  %-16s times %d db 0\n",s->name,s->array_size);
        } else if(s->is_pointer){
            printf("  %-16s dd 0  ; pointer\n",s->name);
        } else {
            if(strcmp(s->type,"int")==0)
                printf("  %-16s dd 0\n",s->name);
            else if(strcmp(s->type,"float")==0)
                printf("  %-16s dq 0.0\n",s->name);
            else if(strcmp(s->type,"char")==0)
                printf("  %-16s db 0\n",s->name);
        }
    }

    /* section .bss */
    printf("\nsection .bss\n");
    for(int i=0;i<temp_count;i++)
        printf("  t%-15d resd 1\n",i);

    /* section .text */
    printf("\nsection .text\n");
    printf("  global main\n");

    for(int i=0;i<tac_count;i++){
        TAC* t=&tac_list[i];
        if(strcmp(t->op,"NOP")==0||strcmp(t->op,"DECL")==0||
           strcmp(t->op,"ARRAY_DECL")==0||strcmp(t->op,"PTR_DECL")==0||
           strcmp(t->op,"STRUCT_DEF")==0) continue;

        if(strcmp(t->op,"FUNC_START")==0){
            printf("\n%s:\n",t->arg1);
            printf("  push ebp\n");
            printf("  mov  ebp, esp\n");
            continue;
        }
        if(strcmp(t->op,"FUNC_END")==0){
            printf("  pop  ebp\n");
            printf("  ret\n");
            continue;
        }
        if(strcmp(t->op,"PARAM")==0||strcmp(t->op,"PARAM_PTR")==0){
            printf("  ; param %s\n",t->result);
            continue;
        }
        if(strcmp(t->op,"LABEL")==0||strcmp(t->op,"FOR_LABEL")==0||
           strcmp(t->op,"CASE_LABEL")==0||strcmp(t->op,"DEFAULT_LABEL")==0||
           strcmp(t->op,"SWITCH_END")==0||strcmp(t->op,"FOR_END")==0){
            const char* lname = t->arg1[0]?t->arg1:t->result;
            printf("%s:\n",lname);
            continue;
        }
        if(strcmp(t->op,"GOTO")==0){
            printf("  jmp  %s\n",t->result); continue;
        }
        if(strcmp(t->op,"IF_FALSE")==0){
            if(is_number(t->arg1)) printf("  mov  eax, %s\n",t->arg1);
            else printf("  mov  eax, [%s]\n",t->arg1);
            printf("  cmp  eax, 0\n");
            printf("  je   %s\n",t->result);
            continue;
        }
        if(strcmp(t->op,"IF_TRUE")==0){
            if(is_number(t->arg1)) printf("  mov  eax, %s\n",t->arg1);
            else printf("  mov  eax, [%s]\n",t->arg1);
            printf("  cmp  eax, 0\n");
            printf("  jne  %s\n",t->result);
            continue;
        }
        if(strcmp(t->op,"=")==0){
            if(is_number(t->arg1)) printf("  mov  eax, %s\n",t->arg1);
            else printf("  mov  eax, [%s]\n",t->arg1);
            printf("  mov  [%s], eax\n",t->result);
            continue;
        }
        if(strcmp(t->op,"NEG")==0){
            if(is_number(t->arg1)) printf("  mov  eax, %s\n",t->arg1);
            else printf("  mov  eax, [%s]\n",t->arg1);
            printf("  neg  eax\n");
            printf("  mov  [%s], eax\n",t->result);
            continue;
        }
        if(strcmp(t->op,"NOT")==0){
            if(is_number(t->arg1)) printf("  mov  eax, %s\n",t->arg1);
            else printf("  mov  eax, [%s]\n",t->arg1);
            printf("  cmp  eax, 0\n");
            printf("  sete al\n");
            printf("  movzx eax, al\n");
            printf("  mov  [%s], eax\n",t->result);
            continue;
        }
        if(strcmp(t->op,"++")==0){
            printf("  mov  eax, [%s]\n",t->arg1);
            printf("  inc  eax\n");
            printf("  mov  [%s], eax\n",t->result);
            continue;
        }
        if(strcmp(t->op,"--")==0){
            printf("  mov  eax, [%s]\n",t->arg1);
            printf("  dec  eax\n");
            printf("  mov  [%s], eax\n",t->result);
            continue;
        }
        if(strcmp(t->op,"+")==0||strcmp(t->op,"-")==0||
           strcmp(t->op,"*")==0||strcmp(t->op,"/")==0){
            if(is_number(t->arg1)) printf("  mov  eax, %s\n",t->arg1);
            else printf("  mov  eax, [%s]\n",t->arg1);
            if(strcmp(t->op,"+")==0){
                if(is_number(t->arg2)) printf("  add  eax, %s\n",t->arg2);
                else printf("  add  eax, [%s]\n",t->arg2);
            } else if(strcmp(t->op,"-")==0){
                if(is_number(t->arg2)) printf("  sub  eax, %s\n",t->arg2);
                else printf("  sub  eax, [%s]\n",t->arg2);
            } else if(strcmp(t->op,"*")==0){
                if(is_number(t->arg2)) printf("  imul eax, %s\n",t->arg2);
                else printf("  imul eax, [%s]\n",t->arg2);
            } else if(strcmp(t->op,"/")==0){
                printf("  cdq\n");
                if(is_number(t->arg2)){
                    printf("  mov  ecx, %s\n",t->arg2);
                    printf("  idiv ecx\n");
                } else {
                    printf("  idiv dword [%s]\n",t->arg2);
                }
            }
            printf("  mov  [%s], eax\n",t->result);
            continue;
        }
        if(strcmp(t->op,"<")==0||strcmp(t->op,">")==0||
           strcmp(t->op,"<=")==0||strcmp(t->op,">=")==0||
           strcmp(t->op,"==")==0||strcmp(t->op,"!=")==0){
            if(is_number(t->arg1)) printf("  mov  eax, %s\n",t->arg1);
            else printf("  mov  eax, [%s]\n",t->arg1);
            if(is_number(t->arg2)) printf("  cmp  eax, %s\n",t->arg2);
            else printf("  cmp  eax, [%s]\n",t->arg2);
            if     (strcmp(t->op,"<" )==0) printf("  setl  al\n");
            else if(strcmp(t->op,">" )==0) printf("  setg  al\n");
            else if(strcmp(t->op,"<=")==0) printf("  setle al\n");
            else if(strcmp(t->op,">=")==0) printf("  setge al\n");
            else if(strcmp(t->op,"==")==0) printf("  sete  al\n");
            else if(strcmp(t->op,"!=")==0) printf("  setne al\n");
            printf("  movzx eax, al\n");
            printf("  mov  [%s], eax\n",t->result);
            continue;
        }
        if(strcmp(t->op,"ARRAY_LOAD")==0){
            printf("  ; %s = %s[%s]\n",t->result,t->arg1,t->arg2);
            printf("  mov  ebx, [%s]\n",t->arg2);
            printf("  lea  ecx, [%s]\n",t->arg1);
            printf("  mov  eax, [ecx + ebx*4]\n");
            printf("  mov  [%s], eax\n",t->result);
            continue;
        }
        if(strcmp(t->op,"ARRAY_STORE")==0){
            printf("  ; %s[%s] = %s\n",t->result,t->arg2,t->arg1);
            printf("  mov  ebx, [%s]\n",t->arg2);
            printf("  lea  ecx, [%s]\n",t->result);
            if(is_number(t->arg1)) printf("  mov  eax, %s\n",t->arg1);
            else printf("  mov  eax, [%s]\n",t->arg1);
            printf("  mov  [ecx + ebx*4], eax\n");
            continue;
        }
        if(strcmp(t->op,"PTR_LOAD")==0){
            printf("  mov  eax, [%s]\n",t->arg1);
            printf("  mov  eax, [eax]\n");
            printf("  mov  [%s], eax\n",t->result);
            continue;
        }
        if(strcmp(t->op,"PTR_STORE")==0){
            printf("  mov  eax, [%s]\n",t->result);
            if(is_number(t->arg1)) printf("  mov  dword [eax], %s\n",t->arg1);
            else printf("  mov  ebx, [%s]\n  mov  [eax], ebx\n",t->arg1);
            continue;
        }
        if(strcmp(t->op,"ADDR")==0){
            printf("  lea  eax, [%s]\n",t->arg1);
            printf("  mov  [%s], eax\n",t->result);
            continue;
        }
        if(strcmp(t->op,"ARG")==0){
            if(is_number(t->arg2)) printf("  push %s\n",t->arg2);
            else printf("  push dword [%s]\n",t->arg2);
            continue;
        }
        if(strcmp(t->op,"CALL")==0){
            printf("  call %s\n",t->arg1);
            printf("  mov  [%s], eax\n",t->result);
            continue;
        }
        if(strcmp(t->op,"BREAK")==0){
            printf("  ; break -> jmp to end label\n");
            continue;
        }
        if(strcmp(t->op,"RETURN")==0){
            if(strlen(t->arg1)>0){
                if(is_number(t->arg1)) printf("  mov  eax, %s\n",t->arg1);
                else printf("  mov  eax, [%s]\n",t->arg1);
            }
            printf("  pop  ebp\n");
            printf("  ret\n");
            continue;
        }
        if(strcmp(t->op,"PRINT")==0){
            printf("  ; printf(%s",t->arg1);
            if(strlen(t->arg2)>0) printf(", %s",t->arg2);
            printf(")\n");
            if(strlen(t->arg2)>0){
                if(is_number(t->arg2)) printf("  push %s\n",t->arg2);
                else printf("  push dword [%s]\n",t->arg2);
            }
            printf("  push %s\n",t->arg1);
            printf("  call printf\n");
            int args=(strlen(t->arg2)>0)?2:1;
            printf("  add  esp, %d\n",args*4);
            continue;
        }
    }
}

/* ============================================
   MAIN
   ============================================ */
int main(){
    FILE* f = fopen("test.c","r");
    if(!f){
        printf("ERROR: Cannot open test.c\n");
        return 1;
    }

    printf("\n==================================================================\n");
    printf("                    MINI C COMPILER (Extended)\n");
    printf("==================================================================\n");
    printf("\n  Reading source file: test.c\n");
    print_line();

    char line_buf[256];
    int lno=1;
    while(fgets(line_buf,sizeof(line_buf),f))
        printf("  %3d | %s",lno++,line_buf);
    printf("\n");
    print_line();

    rewind(f);
    yyin=f;
    yyparse();
    fclose(f);

    print_token_table();
    print_symbol_table();
    print_token_stream();
    print_ast_section();
    print_tac();
    optimize_tac();
    print_optimized_tac();
    generate_assembly();

    printf("\n==================================================================\n");
    printf("  COMPILATION COMPLETE\n");
    printf("==================================================================\n");
    printf("  Total Tokens  : %d\n",token_count);
    printf("  Total Symbols : %d\n",symbol_count);
    printf("  Total Errors  : %d\n",error_count);
    printf("  Total TAC     : %d\n",tac_count);
    printf("==================================================================\n\n");

    return 0;
}