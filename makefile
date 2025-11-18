CC      = gcc
CFLAGS  = -Wall -Wextra -Wpedantic -std=c11
GTK     = $(shell pkg-config --cflags --libs gtk4)

SRC_DIR = src
POL_DIR = policies
GUI_DIR = gui
OBJ_DIR = obj

SRCS = $(wildcard $(SRC_DIR)/*.c)
POLS = $(wildcard $(POL_DIR)/*.c)
GUIS = $(wildcard $(GUI_DIR)/*.c)

OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS)) \
       $(patsubst $(POL_DIR)/%.c,$(OBJ_DIR)/%.o,$(POLS)) \
       $(patsubst $(GUI_DIR)/%.c,$(OBJ_DIR)/%.o,$(GUIS))

TARGET = scheduler_gui

all: dirs $(TARGET)

dirs:
	@mkdir -p $(OBJ_DIR)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(GTK)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) $(shell pkg-config --cflags gtk4) -Iinclude -Igui -c $< -o $@

$(OBJ_DIR)/%.o: $(POL_DIR)/%.c
	$(CC) $(CFLAGS) $(shell pkg-config --cflags gtk4) -Iinclude -Igui -c $< -o $@

$(OBJ_DIR)/%.o: $(GUI_DIR)/%.c
	$(CC) $(CFLAGS) $(shell pkg-config --cflags gtk4) -Iinclude -Igui -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(TARGET)

run: all
	./$(TARGET)

.PHONY: all clean run dirs