/* SPDX-FileCopyrightText: 2025 Sanavi Sonwane
   SPDX-License-Identifier: 0BSD
*/

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <wordexp.h>

#define INITIAL_BUFFER_SIZE 4096
#define HISTORY_LIMIT 10
#define HISTORY_FILE_NAME "/.clipyank_history"

// Function prototypes
char* get_history_path();
void add_to_history(const char *data, size_t data_size);
void show_history();

void send_to_clipboard(const char *copy_cmd) {
    char *buffer = malloc(INITIAL_BUFFER_SIZE);
    if (buffer == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    size_t capacity = INITIAL_BUFFER_SIZE;
    size_t size = 0;
    size_t n;

    while ((n = fread(buffer + size, 1, capacity - size, stdin)) > 0) {
        size += n;
        if (size == capacity) {
            capacity *= 2;
            char *new_buffer = realloc(buffer, capacity);
            if (new_buffer == NULL) {
                perror("realloc");
                free(buffer);
                exit(EXIT_FAILURE);
            }
            buffer = new_buffer;
        }
    }

    if (size > 0) {
        FILE *clipboard_process = popen(copy_cmd, "w");
        if (clipboard_process == NULL) {
            if (errno == ENOENT) {
                fprintf(stderr, "Error: Command not found. Is '%s' installed and in your PATH?\n", copy_cmd);
            } else {
                perror("popen");
            }
            free(buffer);
            exit(EXIT_FAILURE);
        }
        fwrite(buffer, 1, size, clipboard_process);
        pclose(clipboard_process);

        add_to_history(buffer, size);
    }

    fwrite(buffer, 1, size, stdout);
    free(buffer);
}

void receive_from_clipboard(const char *paste_cmd) {
    char buffer[INITIAL_BUFFER_SIZE];
    size_t n;
    FILE *clipboard_process = popen(paste_cmd, "r");
    if (clipboard_process == NULL) {
        if (errno == ENOENT) {
            fprintf(stderr, "Error: Command not found. Is '%s' installed and in your PATH?\n", paste_cmd);
        } else {
            perror("popen");
        }
        exit(EXIT_FAILURE);
    }
    while ((n = fread(buffer, 1, sizeof(buffer), clipboard_process)) > 0) {
        fwrite(buffer, 1, n, stdout);
    }
    pclose(clipboard_process);
}

char* get_history_path() {
    wordexp_t p;
    if (wordexp("~" HISTORY_FILE_NAME, &p, 0) != 0) {
        fprintf(stderr, "Error: Failed to expand home directory path.\n");
        exit(EXIT_FAILURE);
    }
    char *path = strdup(p.we_wordv[0]);
    wordfree(&p);
    return path;
}

void add_to_history(const char *data, size_t data_size) {
    char *history_path = get_history_path();
    char *tmp_history_path = malloc(strlen(history_path) + 5);
    sprintf(tmp_history_path, "%s.tmp", history_path);

    FILE *history_file = fopen(history_path, "r");
    FILE *tmp_file = fopen(tmp_history_path, "w");

    if (tmp_file == NULL) {
        perror("fopen tmp_file");
        free(history_path);
        free(tmp_history_path);
        return;
    }

    // Write new entry to tmp file
    fprintf(tmp_file, "%zu\n", data_size);
    fwrite(data, 1, data_size, tmp_file);
    fprintf(tmp_file, "\n");

    // Copy old entries from history file to tmp file
    if (history_file != NULL) {
        char *line = NULL;
        size_t len = 0;
        int count = 1;
        while (getline(&line, &len, history_file) != -1 && count < HISTORY_LIMIT) {
            size_t entry_size = atol(line);
            fprintf(tmp_file, "%s", line);
            char *entry_data = malloc(entry_size + 1);
            if (fread(entry_data, 1, entry_size, history_file) == entry_size) {
                fwrite(entry_data, 1, entry_size, tmp_file);
            }
            free(entry_data);
            getline(&line, &len, history_file); // Consume newline
            fprintf(tmp_file, "\n");
            count++;
        }
        free(line);
        fclose(history_file);
    }

    fclose(tmp_file);
    rename(tmp_history_path, history_path);

    free(history_path);
    free(tmp_history_path);
}

void show_history() {
    char *history_path = get_history_path();
    FILE *history_file = fopen(history_path, "r");
    if (history_file == NULL) {
        // No history yet, which is not an error.
        free(history_path);
        return;
    }

    char *line = NULL;
    size_t len = 0;
    while (getline(&line, &len, history_file) != -1) {
        size_t entry_size = atol(line);
        char *entry_data = malloc(entry_size + 1);
        if (fread(entry_data, 1, entry_size, history_file) == entry_size) {
            fwrite(entry_data, 1, entry_size, stdout);
            printf("\n--------------------\n");
        }
        free(entry_data);
        getline(&line, &len, history_file); // Consume newline
    }

    free(line);
    fclose(history_file);
    free(history_path);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s --send|--recv|--show\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (strcmp(argv[1], "--show") == 0) {
        show_history();
        return EXIT_SUCCESS;
    }

    char *session_type = getenv("XDG_SESSION_TYPE");
    char *copy_cmd, *paste_cmd;

    if (session_type != NULL && strcmp(session_type, "wayland") == 0) {
        copy_cmd = "wl-copy";
        paste_cmd = "wl-paste";
    } else {
        copy_cmd = "xclip -selection clipboard";
        paste_cmd = "xclip -selection clipboard -o";
    }

    if (strcmp(argv[1], "--send") == 0) {
        send_to_clipboard(copy_cmd);
    } else if (strcmp(argv[1], "--recv") == 0) {
        receive_from_clipboard(paste_cmd);
    } else {
        fprintf(stderr, "Usage: %s --send|--recv|--show\n", argv[0]);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
