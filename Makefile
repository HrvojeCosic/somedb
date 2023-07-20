#=================================================================================================================
#### GENERAL VARIABLES
#=================================================================================================================
BINARY=      bin
CODEDIR=     src
INCDIR=      ./include
TEST_DIR=    test
BUILD_DIR=   build
CXX=         g++
OPT=         -O0
CPP_VER=     -std=c++20

#=================================================================================================================
#### FLAGS & FILES
#=================================================================================================================
CFLAGS=-Wall -Wextra -Wno-missing-braces -g -I $(INCDIR) $(OPT) $(DEPFLAGS)

#Generate files including make rules for .h deps
DEPFLAGS=-MP -MD

CFILES=$(wildcard $(CODEDIR)/*.c)
CPPFILES=$(wildcard $(CODEDIR)/*.cpp)
OFILES_C=$(patsubst %.c, $(BUILD_DIR)/%.o, $(notdir $(CFILES)))
OFILES_CPP=$(patsubst %.cpp, $(BUILD_DIR)/%.o, $(notdir $(CPPFILES)))
CPPFILES=$(foreach DIR,$(CODEDIR),$(wildcard $(DIR)/*.cpp))
DEPFILES=$(patsubst %.c, %.d, $(CFILES))
-include $(DEPFILES)

#=================================================================================================================
#### BUILD
#=================================================================================================================
all: $(BINARY)

$(BINARY): $(OFILES_C) $(OFILES_CPP)
	$(CXX) -o $@ $^

$(BUILD_DIR)/%.o: $(CODEDIR)/%.c | $(BUILD_DIR)
	$(CXX) $(CFLAGS) -c $< -o $@ 

$(BUILD_DIR)/%.o: $(CODEDIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CPP_VER) $(CFLAGS) -c $< -o $@ 

$(BUILD_DIR): 
	mkdir $@

#=================================================================================================================
#### TEST
#=================================================================================================================
TESTS_C=      $(wildcard $(TEST_DIR)/*.c)
TESTS_CPP=    $(wildcard $(TEST_DIR)/*.cpp)
TESTS=        $(TESTS_C) $(TESTS_CPP)

TESTBINS_C=   $(patsubst $(TEST_DIR)/%.c, $(TEST_DIR)/bin/%, $(TESTS_C))
TESTBINS_CPP= $(patsubst $(TEST_DIR)/%.cpp, $(TEST_DIR)/bin/%, $(TESTS_CPP))
TESTBINS=     $(TESTBINS_C) $(TESTBINS_CPP)

TESTFLAGS=  -fsanitize=thread -ltsan
GTEST_LIBS= -lgtest -lgtest_main -lpthread
CHECK_LIBS= -lcheck -lsubunit -lm


test: $(TEST_DIR)/bin $(TESTBINS)
	@for test in $(TESTBINS) ; do ./$$test ; done

$(TEST_DIR)/bin/%: $(TEST_DIR)/%.c $(CFILES)
	$(CXX) $(CFLAGS) -o $@ $< $(filter-out src/main.c,$(CFILES)) $(CHECK_LIBS) $(TESTFLAGS)

$(TEST_DIR)/bin/%: $(TEST_DIR)/%.cpp $(CPPFILES)
	$(CXX) $(CFLAGS) -o $@ $< $(filter-out src/main.cpp,$(CPPFILES)) $(GTEST_LIBS) $(TESTFLAGS)

$(TEST_DIR)/bin:
	mkdir -p $@

#=================================================================================================================
#### GIT & CLEANUP
#=================================================================================================================
clean:
	rm -rf $(BINARY) $(BUILD_DIR) $(OFILES) $(DEPFILES) $(TESTBINS)

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

#=================================================================================================================
#### PACKAGES SETUP
#=================================================================================================================
install_pckgs: install_libgtest install_check

install_libgtest:
	sudo apt-get install libgtest-dev
	cd /usr/src/gtest; \
		sudo cmake CMakeLists.txt; \
		sudo make; \
		sudo cp ./lib/*.a /usr/lib;

install_check:
	dpkg -l | grep 'check.*unit test framework for C' || (echo "installing Check, C testing framework" && sudo apt-get install check)


