CC = gcc
CFLAGS = -w

all: compiler

compiler: lex.yy.c parser.tab.c parser.tab.h main.c
	$(CC) $(CFLAGS) -o compiler lex.yy.c parser.tab.c main.c

lex.yy.c: lexer.l parser.tab.h
	flex lexer.l

parser.tab.c parser.tab.h: parser.y
	bison -d parser.y

clean:
	rm -f compiler lex.yy.c parser.tab.c parser.tab.h

run: compiler
	./compiler