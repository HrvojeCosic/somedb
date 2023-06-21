BINARY=bin
CODEDIRS=. src
INCDIRS=. ./include
TEST=test

CC=gcc
OPT=-O0
#Generate files including make rules for .h deps
DEPFLAGS=-MP -MD 
CFLAGS=-Wall -Wextra -Wno-missing-braces -g $(foreach DIR,$(INCDIRS),-I$(DIR)) $(OPT) $(DEPFLAGS)
TESTFLAGS=-lcheck -lsubunit -lm

CFILES=$(foreach DIR,$(CODEDIRS),$(wildcard $(DIR)/*.c))
OFILES=$(patsubst %.c,%.o,$(CFILES))
DEPFILES=$(patsubst %.c,%.d,$(CFILES))

TESTS=$(wildcard $(TEST)/*.c)
TESTBINS=$(patsubst $(TEST)/%.c, $(TEST)/bin/%, $(TESTS))

all: $(BINARY)

$(BINARY): $(OFILES)
	$(CC) -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

test: $(TEST)/bin $(TESTBINS)
	@for test in $(TESTBINS) ; do ./$$test ; done

$(TEST)/bin/%: $(TEST)/%.c $(CFILES)
	$(CC) $(CFLAGS) -o $@ $< $(filter-out src/main.c,$(CFILES)) $(TESTFLAGS)

$(TEST)/bin:
	mkdir $@

clean:
	rm -rf $(BINARY) $(OFILES) $(DEPFILES) $(TEST)/bin $(TESTBINS)

-include $(DEPFILES)
