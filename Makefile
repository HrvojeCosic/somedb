#=================================================================================================================
#### GENERAL VARIABLES
#=================================================================================================================
BINARY=      bin
DBFILE=      dbfile.txt
CODEDIR=     src
INCDIR=      ./include
TEST=        test
BUILD_DIR=   build
CC=          g++
OPT=         -O0

#=================================================================================================================
#### FLAGS & FILES
#=================================================================================================================
CFLAGS=-Wall -Wextra -Wno-missing-braces -g -I $(INCDIR) $(OPT) $(DEPFLAGS)
TESTFLAGS=-lcheck -lsubunit -lm -fsanitize=thread -ltsan
#Generate files including make rules for .h deps
DEPFLAGS=-MP -MD

CFILES=$(wildcard $(CODEDIR)/*.c)
CPPFILES=$(wildcard $(CODEDIR)/*.cpp)
OFILES_C=$(patsubst %.c, $(BUILD_DIR)/%.o, $(notdir $(CFILES)))
OFILES_CPP=$(patsubst %.cpp, $(BUILD_DIR)/%.o, $(notdir $(CPPFILES)))
CPPFILES=$(foreach DIR,$(CODEDIR),$(wildcard $(DIR)/*.cpp))
DEPFILES=$(patsubst %.c, %.d, $(CFILES))

#=================================================================================================================
#### BUILD
#=================================================================================================================
all: $(BINARY)

$(BINARY): $(OFILES_C) $(OFILES_CPP)
	$(CC) -o $@ $^

$(BUILD_DIR)/%.o: $(CODEDIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@ 

$(BUILD_DIR)/%.o: $(CODEDIR)/%.cpp | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@ 

$(BUILD_DIR): 
	mkdir $@

#=================================================================================================================
#### TEST
#=================================================================================================================
TESTS=$(wildcard $(TEST)/*.c)
TESTBINS=$(patsubst $(TEST)/%.c, $(TEST)/bin/%, $(TESTS))

test: $(TEST)/bin $(TESTBINS)
	@for test in $(TESTBINS) ; do ./$$test ; done

$(TEST)/bin/%: $(TEST)/%.c $(CFILES)
	$(CC) $(CFLAGS) -o $@ $< $(filter-out src/main.c,$(CFILES)) $(TESTFLAGS)

$(TEST)/bin:
	mkdir $@

#=================================================================================================================
#### GIT & CLEANUP
#=================================================================================================================
clean:
	rm -rf $(BINARY) $(BUILD_DIR) $(OFILES) $(DEPFILES) $(TEST)/bin $(TESTBINS) $(DBFILE)

diff:
	$(shell find . -iname '*.h' -o -iname '*.c' -o -iname '*.cpp' -o -iname '*.hpp' | xargs clang-format -i)
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
