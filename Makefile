SRC_DIR = src
BIN_DIR = bin
LOG_DIR = log
DOC_DIR = doc
TESTS_DIR = test
DOXYGEN_ENABLED = 1

.PHONY: all tests info format clean doxygen

CC = gcc-12
CXX = g++-12
CFLAGS = -pthread -Wall -Werror -O0 -g --std=gnu2x
CXXFLAGS = -pthread -Wall -Werror -O0 -g --std=c++20

# tells it to search for 
CPPFLAGS = -I $(SRC_DIR)


DOXYGEN_FILES = $(SRCS) $(HDRS) Doxyfile

DOC_MARKER = $(DOC_DIR)/.doxygen_marker

# add each test name to this list
# for example:
# TEST_MAINS = $(TESTS_DIR)/test1.c $(TESTS_DIR)/othertest.c $(TESTS_DIR)/sched-demo.c
TEST_MAINS = $(TESTS_DIR)/sched-demo.c 

# list all files with their own main() function here
# for example:
# MAIN_FILES = $(SRC_DIR)/stand_alone_pennfat.c $(SRC_DIR)/helloworld.c $(SRC_DIR)/pennos.c
MAIN_FILES = $(SRC_DIR)/pennos.c

# to get the executables, remove the .c from the filename and put 
# it in the BIN_DIR
EXECS = $(subst $(SRC_DIR),$(BIN_DIR),$(MAIN_FILES:.c=))
TEST_EXECS = $(subst $(TESTS_DIR),$(BIN_DIR),$(TEST_MAINS:.c=))

# srcs = all C files in SRC_DIR that are not listed in MAIN_FILES
SRCS = $(filter-out $(MAIN_FILES), $(shell find $(SRC_DIR) -type f -name '*.c'))
HDRS = $(shell find src -type f -name '*.h')
OBJS = $(SRCS:.c=.o)

TEST_OBJS = $($(wildcard $(TESTS_DIR)/*.c):.c=.o)

all: $(EXECS) doxygen

tests: $(TEST_EXECS)

$(EXECS): $(BIN_DIR)/%: $(SRC_DIR)/%.c $(OBJS) $(HDRS)
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $(OBJS) $<

$(TEST_EXECS): $(BIN_DIR)/%: $(TESTS_DIR)/%.c $(OBJS) $(HDRS)
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $(OBJS) $(subst $(BIN_DIR)/,$(TESTS_DIR)/,$@).c

%.o: %.c $(HDRS)
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ -c $<

%.o: %.cpp $(HDRS)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $@ -c $<

info:
	$(info MAIN_FILES: $(MAIN_FILES)) \
	$(info EXECS: $(EXECS)) \
	$(info SRCS: $(SRCS)) \
	$(info HDRS: $(HDRS)) \
	$(info OBJS: $(OBJS)) \
	$(info TEST_MAINS: $(TEST_MAINS)) \
	$(info TEST_EXECS: $(TEST_EXECS))

format:
	clang-format -i --verbose --style=Chromium $(MAIN_FILES) $(TEST_MAINS) $(SRCS) $(HDRS)

$(DOC_MARKER): $(DOXYGEN_FILES)
	@if [ "$(DOXYGEN_ENABLED)" = "1" ]; then \
		command -v doxygen >/dev/null 2>&1 || { echo "Doxygen not installed, please install it -maya"; exit 0; }; \
		echo "Running Doxygen in the background..."; \
		( nohup doxygen Doxyfile > /dev/null 2>&1 && touch $(DOC_MARKER)) & \
	fi

doxygen:$(DOC_MARKER)

clean:
	rm $(OBJS) $(EXECS) $(TEST_EXECS)
