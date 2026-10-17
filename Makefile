CC ?= cc
AR ?= ar
BUILD_DIR ?= build
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -pedantic -O2
CPPFLAGS ?= -Iinclude -Isrc

LIB_SRCS := \
	src/kdtl_accounts.c \
	src/kdtl_amount.c \
	src/kdtl_catalog.c \
	src/kdtl_codec.c \
	src/kdtl_engine.c \
	src/kdtl_events.c \
	src/kdtl_ids.c \
	src/kdtl_limits.c \
	src/kdtl_positions.c \
	src/kdtl_policy.c \
	src/kdtl_reconcile.c \
	src/kdtl_reporting.c \
	src/kdtl_settlement.c \
	src/kdtl_stress.c

LIB_OBJS := $(patsubst src/%.c,$(BUILD_DIR)/%.o,$(LIB_SRCS))
SCENARIO := $(BUILD_DIR)/kdtl_scenario
STATIC_LIB := $(BUILD_DIR)/libkeystonedtl.a

.PHONY: all clean test

all: $(SCENARIO)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: src/%.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/kdtl_scenario.o: tools/kdtl_scenario.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(STATIC_LIB): $(LIB_OBJS)
	$(AR) rcs $@ $^

$(SCENARIO): $(STATIC_LIB) $(BUILD_DIR)/kdtl_scenario.o
	$(CC) $(CFLAGS) $(BUILD_DIR)/kdtl_scenario.o $(STATIC_LIB) -o $@

test: all
	node --test "tests/node/*.test.js"

clean:
	rm -rf $(BUILD_DIR)
