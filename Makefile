CC     = gcc
FLEX   = flex
BISON  = bison
CFLAGS = -w -g

TARGET = compiler

all: $(TARGET)

# Step 1: bison আগে run করতে হবে - parser.tab.h তৈরি হবে
parser.tab.c parser.tab.h: parser.y
	$(BISON) -d parser.y

# Step 2: flex পরে run করবে - parser.tab.h দরকার
lex.yy.c: lexer.l parser.tab.h
	$(FLEX) lexer.l

# Step 3: সব একসাথে compile
$(TARGET): parser.tab.c parser.tab.h lex.yy.c main.c compiler.h
	$(CC) $(CFLAGS) -o $(TARGET) parser.tab.c lex.yy.c main.c -lm

run: $(TARGET)
	./$(TARGET)

output: $(TARGET)
	./$(TARGET) > output.txt
	@echo "Output saved to output.txt"

clean:
	rm -f $(TARGET) parser.tab.c parser.tab.h lex.yy.c output.txt

.PHONY: all run clean output