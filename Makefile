# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c99

# Project files
TARGET = clipyank
SOURCE = src/clipyank.c
HISTORY_FILE = ~/.clipyank_history

# Phony targets
.PHONY: all clean install uninstall

# Default target: build the executable
all: $(TARGET)

# Rule to build the executable
$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCE)

# Remove build artifacts
clean:
	rm -f $(TARGET)

# Install the executable and create the history file
install: all
	sudo install -m 0755 $(TARGET) /usr/local/bin
	touch $(HISTORY_FILE)

# Uninstall the executable and remove the history file
uninstall:
	sudo rm -f /usr/local/bin/$(TARGET)
	rm -f $(HISTORY_FILE)