BINARY=bin
DBFILE=dbfile.txt
CODEDIRS=. src
INCDIRS=. ./include
TEST=test

CC=g++
OPT=-O0
#Generate files including make rules for .h deps
DEPFLAGS=-MP -MD 
CFLAGS=-Wall -Wextra -Wno-missing-braces -g $(foreach DIR,$(INCDIRS),-I$(DIR)) $(OPT) $(DEPFLAGS)
TESTFLAGS=-lcheck -lsubunit -lm -fsanitize=thread -ltsan

CMDFORMAT=$(shell find . -iname '*.h' -o -iname '*.c' -o -iname '*.cpp' -o -iname '*.hpp' | xargs clang-format -i)

CFILES=$(foreach DIR,$(CODEDIRS),$(wildcard $(DIR)/*.c))
OFILES=$(patsubst %.c,%.o,$(CFILES))
DEPFILES=$(patsubst %.c,%.d,$(CFILES))

TESTS=$(wildcard $(TEST)/*.c)
TESTBINS=$(patsubst $(TEST)/%.c, $(TEST)/bin/%, $(TESTS))

all: $(BINARY)

$(BINARY): $(OFILES)
	$(CC) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

test: $(TEST)/bin $(TESTBINS)
	@for test in $(TESTBINS) ; do ./$$test ; done

$(TEST)/bin/%: $(TEST)/%.c $(CFILES)
	$(CC) $(CFLAGS) -o $@ $< $(filter-out src/main.c,$(CFILES)) $(TESTFLAGS)

$(TEST)/bin:
	mkdir $@

clean:
	rm -rf $(BINARY) $(OFILES) $(DEPFILES) $(TEST)/bin $(TESTBINS) $(DBFILE)

diff:
	$(CMDFORMAT)
	@git diff
	@git status

commit:
	@if [ -z "$(m)" ]; then \
		echo "param 'm' is not provided. Run: make commit m='My comment'"; \
		exit 1; \
	fi
	git add .
	git commit -m "$(m)"
	git push

-include $(DEPFILES)
