CC = gcc
CFLAGS = -O3 -fopenmp -Wall -Wextra -std=c99
LDFLAGS = -lm -fopenmp

SRC_DIR = src
OBJ_DIR = build
BIN_DIR = bin
TARGET = $(BIN_DIR)/img_processor

SRCS = $(SRC_DIR)/main.c $(SRC_DIR)/filters.c $(SRC_DIR)/pipeline.c $(SRC_DIR)/timer.c
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

all: $(TARGET)

$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CC) $(OBJS) $(LDFLAGS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -I$(SRC_DIR) -c $< -o $@

$(BIN_DIR) $(OBJ_DIR):
	mkdir -p $@

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR) data/output/*

run: $(TARGET)
	./$(TARGET) --help

test: $(TARGET)
	@echo "Ejecutando test de verificacion..."
	python3 scripts/benchmark.py --quick

.PHONY: all clean run test
