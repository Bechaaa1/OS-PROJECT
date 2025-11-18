# Makefile - Scheduler Project (fixed & working)
CC      = gcc
CFLAGS  = -Wall -Wextra -Wpedantic -std=c11
INC     = -Iinclude
SRC_DIR = src
POL_DIR = policies
OBJ_DIR = obj

# All .c files
SRCS = $(wildcard $(SRC_DIR)/*.c)
POLS = $(wildcard $(POL_DIR)/*.c)

# Corresponding .o files
OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS)) \
       $(patsubst $(POL_DIR)/%.c,$(OBJ_DIR)/%.o,$(POLS))

TARGET = scheduler

# Default target
all: dirs $(TARGET)

# Create obj directory if missing
dirs:
	@mkdir -p $(OBJ_DIR)

# Link executable
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@

# Compile files from src/
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

# Compile policy files
$(OBJ_DIR)/%.o: $(POL_DIR)/%.c
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

# Clean
clean:
	rm -rf $(OBJ_DIR) $(TARGET)

# Run with example config
run: all
	./$(TARGET) config/example.txt

# Phony targets
.PHONY: all clean run dirs