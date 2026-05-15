%{
#include "compiler.h"
#include "parser.tab.h"

int yyline = 1;
%}

%option noyywrap

/* ---- Patterns ---- */
DIGIT       [0-9]
LETTER      [a-zA-Z_]
ID          {LETTER}({LETTER}|{DIGIT})*
NUMBER      {DIGIT}+(\.{DIGIT}+)?
STRING      \"[^\"]*\"
WHITESPACE  [ \t\r]+

%%

"int"       { add_token("int",    yytext, TOK_KEYWORD, yyline); return INT;    }
"float"     { add_token("float",  yytext, TOK_KEYWORD, yyline); return FLOAT;  }
"char"      { add_token("char",   yytext, TOK_KEYWORD, yyline); return CHAR;   }
"if"        { add_token("if",     yytext, TOK_KEYWORD, yyline); return IF;     }
"else"      { add_token("else",   yytext, TOK_KEYWORD, yyline); return ELSE;   }
"while"     { add_token("while",  yytext, TOK_KEYWORD, yyline); return WHILE;  }
"for"       { add_token("for",    yytext, TOK_KEYWORD, yyline); return FOR;    }
"return"    { add_token("return", yytext, TOK_KEYWORD, yyline); return RETURN; }
"void"      { add_token("void",   yytext, TOK_KEYWORD, yyline); return VOID;   }
"printf"    { add_token("printf", yytext, TOK_KEYWORD, yyline); return PRINTF; }

{NUMBER}    { add_token("NUMBER", yytext, TOK_NUMBER,     yyline);
              yylval.str = strdup(yytext); return NUMBER; }

{ID}        { add_token("ID",     yytext, TOK_IDENTIFIER, yyline);
              yylval.str = strdup(yytext); return ID; }

{STRING}    { add_token("STRING", yytext, TOK_STRING,     yyline);
              yylval.str = strdup(yytext); return STRING_LIT; }

"+"         { add_token("+",  yytext, TOK_OPERATOR,    yyline); return PLUS;   }
"-"         { add_token("-",  yytext, TOK_OPERATOR,    yyline); return MINUS;  }
"*"         { add_token("*",  yytext, TOK_OPERATOR,    yyline); return MUL;    }
"/"         { add_token("/",  yytext, TOK_OPERATOR,    yyline); return DIV;    }
"="         { add_token("=",  yytext, TOK_OPERATOR,    yyline); return ASSIGN; }
"=="        { add_token("==", yytext, TOK_OPERATOR,    yyline); return EQ;     }
"!="        { add_token("!=", yytext, TOK_OPERATOR,    yyline); return NEQ;    }
"<"         { add_token("<",  yytext, TOK_OPERATOR,    yyline); return LT;     }
">"         { add_token(">",  yytext, TOK_OPERATOR,    yyline); return GT;     }
"<="        { add_token("<=", yytext, TOK_OPERATOR,    yyline); return LEQ;    }
">="        { add_token(">=", yytext, TOK_OPERATOR,    yyline); return GEQ;    }
"&&"        { add_token("&&", yytext, TOK_OPERATOR,    yyline); return AND;    }
"||"        { add_token("||", yytext, TOK_OPERATOR,    yyline); return OR;     }
"!"         { add_token("!",  yytext, TOK_OPERATOR,    yyline); return NOT;    }

";"         { add_token(";",  yytext, TOK_PUNCTUATION, yyline); return SEMI;   }
","         { add_token(",",  yytext, TOK_PUNCTUATION, yyline); return COMMA;  }
"("         { add_token("(",  yytext, TOK_PUNCTUATION, yyline); return LPAREN; }
")"         { add_token(")",  yytext, TOK_PUNCTUATION, yyline); return RPAREN; }
"{"         { add_token("{",  yytext, TOK_PUNCTUATION, yyline); return LBRACE; }
"}"         { add_token("}",  yytext, TOK_PUNCTUATION, yyline); return RBRACE; }

\n          { yyline++; }
{WHITESPACE} { /* skip */ }
"//".*      { /* skip single-line comment */ }

.           { 
              add_token("UNKNOWN", yytext, TOK_UNKNOWN, yyline);
              printf("[LEX ERROR] Unknown char '%s' at line %d\n", yytext, yyline);
              error_count++;
            }

%%