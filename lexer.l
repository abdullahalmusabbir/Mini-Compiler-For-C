%{
#include "compiler.h"
#include "parser.tab.h"

int yyline = 1;
%}

%option noyywrap

DIGIT     [0-9]
LETTER    [a-zA-Z_]
ID        {LETTER}({LETTER}|{DIGIT})*
NUMBER    {DIGIT}+(\.{DIGIT}+)?
STRING    \"[^\"]*\"
WHITESPACE [ \t\r]+

%%

"/*"    {
            /* multi-line comment skip */
            int c;
            while(1) {
                c = input();
                if(c == EOF) break;
                if(c == '\n') yyline++;
                if(c == '*') {
                    c = input();
                    if(c == '/') break;
                    if(c == '\n') yyline++;
                }
            }
        }

"//".*  { /* single-line comment skip */ }

"int"       { add_token("int",      yytext, TOK_KEYWORD, yyline); return INT;    }
"float"     { add_token("float",    yytext, TOK_KEYWORD, yyline); return FLOAT;  }
"char"      { add_token("char",     yytext, TOK_KEYWORD, yyline); return CHAR;   }
"void"      { add_token("void",     yytext, TOK_KEYWORD, yyline); return VOID;   }
"if"        { add_token("if",       yytext, TOK_KEYWORD, yyline); return IF;     }
"else"      { add_token("else",     yytext, TOK_KEYWORD, yyline); return ELSE;   }
"while"     { add_token("while",    yytext, TOK_KEYWORD, yyline); return WHILE;  }
"for"       { add_token("for",      yytext, TOK_KEYWORD, yyline); return FOR;    }
"return"    { add_token("return",   yytext, TOK_KEYWORD, yyline); return RETURN; }
"printf"    { add_token("printf",   yytext, TOK_KEYWORD, yyline); return PRINTF; }
"struct"    { add_token("struct",   yytext, TOK_KEYWORD, yyline); return STRUCT; }
"switch"    { add_token("switch",   yytext, TOK_KEYWORD, yyline); return SWITCH; }
"case"      { add_token("case",     yytext, TOK_KEYWORD, yyline); return CASE;   }
"break"     { add_token("break",    yytext, TOK_KEYWORD, yyline); return BREAK;  }
"default"   { add_token("default",  yytext, TOK_KEYWORD, yyline); return DEFAULT;}

{NUMBER}  { add_token("NUMBER", yytext, TOK_NUMBER,     yyline);
            yylval.str = strdup(yytext); return NUMBER; }

{ID}      { add_token("ID",     yytext, TOK_IDENTIFIER, yyline);
            yylval.str = strdup(yytext); return ID; }

{STRING}  { add_token("STRING", yytext, TOK_STRING,     yyline);
            yylval.str = strdup(yytext); return STRING_LIT; }

"+"   { add_token("+",  yytext, TOK_OPERATOR,    yyline); return PLUS;   }
"-"   { add_token("-",  yytext, TOK_OPERATOR,    yyline); return MINUS;  }
"*"   { add_token("*",  yytext, TOK_OPERATOR,    yyline); return MUL;    }
"/"   { add_token("/",  yytext, TOK_OPERATOR,    yyline); return DIV;    }
"="   { add_token("=",  yytext, TOK_OPERATOR,    yyline); return ASSIGN; }
"=="  { add_token("==", yytext, TOK_OPERATOR,    yyline); return EQ;     }
"!="  { add_token("!=", yytext, TOK_OPERATOR,    yyline); return NEQ;    }
"<"   { add_token("<",  yytext, TOK_OPERATOR,    yyline); return LT;     }
">"   { add_token(">",  yytext, TOK_OPERATOR,    yyline); return GT;     }
"<="  { add_token("<=", yytext, TOK_OPERATOR,    yyline); return LEQ;    }
">="  { add_token(">=", yytext, TOK_OPERATOR,    yyline); return GEQ;    }
"&&"  { add_token("&&", yytext, TOK_OPERATOR,    yyline); return AND;    }
"||"  { add_token("||", yytext, TOK_OPERATOR,    yyline); return OR;     }
"!"   { add_token("!",  yytext, TOK_OPERATOR,    yyline); return NOT;    }
"&"   { add_token("&",  yytext, TOK_OPERATOR,    yyline); return AMP;    }
"->"  { add_token("->", yytext, TOK_OPERATOR,    yyline); return ARROW;  }
"++"  { add_token("++", yytext, TOK_OPERATOR,    yyline); return INC;    }
"--"  { add_token("--", yytext, TOK_OPERATOR,    yyline); return DEC;    }

";"   { add_token(";",  yytext, TOK_PUNCTUATION, yyline); return SEMI;   }
","   { add_token(",",  yytext, TOK_PUNCTUATION, yyline); return COMMA;  }
"("   { add_token("(",  yytext, TOK_PUNCTUATION, yyline); return LPAREN; }
")"   { add_token(")",  yytext, TOK_PUNCTUATION, yyline); return RPAREN; }
"{"   { add_token("{",  yytext, TOK_PUNCTUATION, yyline); return LBRACE; }
"}"   { add_token("}",  yytext, TOK_PUNCTUATION, yyline); return RBRACE; }
"["   { add_token("[",  yytext, TOK_PUNCTUATION, yyline); return LBRACKET;}
"]"   { add_token("]",  yytext, TOK_PUNCTUATION, yyline); return RBRACKET;}
"."   { add_token(".",  yytext, TOK_PUNCTUATION, yyline); return DOT;    }
":"   { add_token(":",  yytext, TOK_PUNCTUATION, yyline); return COLON;  }

\n          { yyline++; }
{WHITESPACE} { /* skip */ }

. {
    add_token("UNKNOWN", yytext, TOK_UNKNOWN, yyline);
    printf("[LEX ERROR] Unknown char '%s' at line %d\n", yytext, yyline);
    error_count++;
  }

%%