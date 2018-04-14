
CC = gcc

CFLAGS = -std=c99 -Wall
CFLAGS := $(CFLAGS) -Isrc
DEBUG_CFLAGS = $(CFLAGS) -DDEBUG -g -O0 -Wno-unused-variable -Wno-unused-but-set-variable
RELEASE_CFLAGS = $(CFLAGS) -O3
LDFLAGS = -lm

SRC = $(wildcard src/*.c modules/*.c)
OBJ = $(SRC:.c=.o)
BIN = bin/modelgen

DEBUG_OBJ = $(OBJ:%=bin/debug/%)
DEBUG_BIN = bin/debug/modelgen

RELEASE_OBJ = $(OBJ:%=bin/release/%)
RELEASE_BIN = bin/release/modelgen

TEST_SRC = $(wildcard tests/*.c)
TEST_OBJ = $(TEST_SRC:%.c=bin/debug/%.o)
TEST_BIN = $(TEST_SRC:%.c=bin/debug/%)

all: release

debug: $(DEBUG_BIN)

release: test
release: $(RELEASE_BIN)

test: $(TEST_BIN)

$(DEBUG_BIN): $(DEBUG_OBJ)
	@printf "\e[95mCC\e[39m %s \e[90m%s\e[0m\n" $(BIN) $@
	@$(CC) $^ $(LDFLAGS) -o $@
	@cp $@ $(BIN)

$(RELEASE_BIN): $(RELEASE_OBJ)
	@printf "\e[95mCC\e[39m %s \e[90m%s\e[0m\n" $(BIN) $@
	@$(CC) $^ $(LDFLAGS) -o $@
	@cp $@ $(BIN)

$(TEST_BIN): %: %.o $(filter-out bin/debug/src/modelgen.o, $(DEBUG_OBJ))
	@printf "\e[93mCC\e[39m %s\e[0m\n" $@
	@$(CC) $^ $(LDFLAGS) -o $@
	@./$@

bin/debug/%.o: %.c
	@printf "\e[32mCC\e[39m %s \e[90m%s\e[0m\n" $@ $<
	@mkdir -p $(@D)
	@$(CC) $(DEBUG_CFLAGS) -c $< -o $@

bin/release/%.o: %.c
	@printf "\e[32mCC\e[39m %s \e[90m%s\e[0m\n" $@ $<
	@mkdir -p $(@D)
	@$(CC) $(RELEASE_CFLAGS) -c $< -o $@

clean:
	@rm -rfv bin

.PHONY: debug release test clean
