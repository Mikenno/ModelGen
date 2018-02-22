
CC = gcc

CFLAGS = -std=c99 -Wall
DEBUG_CFLAGS = $(CFLAGS) -DDEBUG -g -O0 -Wno-unused-variable -Wno-unused-but-set-variable
RELEASE_CFLAGS = $(CFLAGS) -O3
LDFLAGS = -lm

SRC = $(wildcard src/*.c)
OBJ = $(SRC:.c=.o)
BIN = bin/modelgen

DEBUG_OBJ = $(OBJ:%=bin/debug/%)
DEBUG_BIN = bin/debug/modelgen

RELEASE_OBJ = $(OBJ:%=bin/release/%)
RELEASE_BIN = bin/release/modelgen

all: release

debug: $(DEBUG_BIN)
release: $(RELEASE_BIN)

$(DEBUG_BIN): $(DEBUG_OBJ)
	@printf "\e[95mCC \e[39m%s \e[90m%s\e[0m\n" $(BIN) $@
	@$(CC) $^ $(LDFLAGS) -o $@
	@cp $@ $(BIN)

$(RELEASE_BIN): $(RELEASE_OBJ)
	@printf "\e[95mCC \e[39m%s \e[90m%s\e[0m\n" $(BIN) $@
	@$(CC) $^ $(LDFLAGS) -o $@
	@cp $@ $(BIN)

bin/debug/%.o: %.c
	@printf "\e[32mCC \e[39m%s \e[90m%s\e[0m\n" $@ $<
	@mkdir -p $(@D)
	@$(CC) $(DEBUG_CFLAGS) -c $< -o $@

bin/release/%.o: %.c
	@printf "\e[32mCC \e[39m%s \e[90m%s\e[0m\n" $@ $<
	@mkdir -p $(@D)
	@$(CC) $(RELEASE_CFLAGS) -c $< -o $@

clean:
	@rm -rfv bin

.PHONY: debug release clean
