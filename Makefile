BINARY=bin
CODEDIRS=. src
INCDIRS=. ./include

CC=gcc
OPT=-O0
#Generate files including make rules for .h deps
DEPFLAGS=-MP -MD 
CFLAGS=-Wall -Wextra -g $(foreach DIR,$(INCDIRS),-I$(DIR)) $(OPT) $(DEPFLAGS)

CFILES=$(foreach DIR,$(CODEDIRS),$(wildcard $(DIR)/*.c))
OFILES=$(patsubst %.c,%.o,$(CFILES))
DEPFILES=$(patsubst %.c,%.d,$(CFILES))

all: $(BINARY)

$(BINARY): $(OFILES)
	$(CC) -o $@ $^

%.o:%.c
	$(CC) $(CFLAGS) -c -o $@ $^

clean:
	rm -rf $(BINARY) $(OFILES) $(DEPFILES)

-include $(DEPFILES)

diff:
	@git status
	@git diff --stat
