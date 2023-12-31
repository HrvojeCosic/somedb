#=================================================================================================================
#### GENERAL VARIABLES
#=================================================================================================================
BINARY=      bin
VPATH=       . src src/disk src/index src/sql src/utils
INCDIRS=     . ./include ./include/disk ./include/index ./include/sql  ./include/utils
TEST_DIR=    test
BUILD_DIR=   build
CXX=         g++
OPT=         -O0
CPP_VER=     -std=c++20
DBFILES_DIR= db_files

#=================================================================================================================
#### FLAGS & FILES
#=================================================================================================================
CFLAGS=-Wall -Wextra -Wno-missing-braces -g $(foreach dir,$(INCDIRS),-I$(dir)) $(OPT) $(DEPFLAGS)

#Generate files including make rules for .h deps
DEPFLAGS=-MP -MD

CFILES=$(foreach dir, $(VPATH), $(wildcard $(dir)/*.c))
CPPFILES=$(foreach dir, $(VPATH), $(wildcard $(dir)/*.cpp))
CPPFILES_NO_MAIN=$(filter-out src/main.cpp,$(CPPFILES))
CFILES_NO_MAIN=$(filter-out src/main.c,$(CFILES))
HPPFILES=$(foreach dir, $(INCDIRS), $(wildcard $(dir)/*.hpp))
OFILES_C=$(patsubst %.c, $(BUILD_DIR)/%.o, $(notdir $(CFILES)))
OFILES_CPP=$(patsubst %.cpp, $(BUILD_DIR)/%.o, $(notdir $(CPPFILES)))
DEPFILES=$(patsubst %.c, %.d, $(CFILES)) $(patsubst %.cpp, %.d, $(CPPFILES))
-include $(DEPFILES)

#=================================================================================================================
#### BUILD
#=================================================================================================================
all: $(BINARY)

$(BINARY): $(OFILES_C) $(OFILES_CPP)
	$(CXX) -o $@ $^

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CXX) $(CFLAGS) -c $< -o $@ 

$(BUILD_DIR)/%.o: %.cpp | $(BUILD_DIR)
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
	$(CXX) $(CFLAGS) -o $@ $< $(CFILES_NO_MAIN) $(CHECK_LIBS) $(TESTFLAGS)

$(TEST_DIR)/bin/%: $(TEST_DIR)/%.cpp $(CPPFILES) $(HPPFILES)
	$(CXX) $(CPP_VER) $(CFLAGS) -o $@ $< $(CPPFILES_NO_MAIN) $(CFILES) $(GTEST_LIBS) $(TESTFLAGS)

$(TEST_DIR)/bin:
	mkdir -p $@

#=================================================================================================================
#### GIT & CLEANUP
#=================================================================================================================
clean:
	rm -rf $(BINARY) $(BUILD_DIR) $(OFILES) $(DEPFILES) $(TESTBINS) $(DBFILES_DIR)

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


