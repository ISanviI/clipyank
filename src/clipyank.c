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
void exchange_clipboard_and_stdin(const char *copy_cmd, const char *paste_cmd);

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

void exchange_clipboard_and_stdin(const char *copy_cmd, const char *paste_cmd) {
    // 1. Read selected text from stdin
    char *stdin_buffer = malloc(INITIAL_BUFFER_SIZE);
    if (stdin_buffer == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    size_t stdin_capacity = INITIAL_BUFFER_SIZE;
    size_t stdin_size = 0;
    size_t n;

    while ((n = fread(stdin_buffer + stdin_size, 1, stdin_capacity - stdin_size, stdin)) > 0) {
        stdin_size += n;
        if (stdin_size == stdin_capacity) {
            stdin_capacity *= 2;
            char *new_stdin_buffer = realloc(stdin_buffer, stdin_capacity);
            if (new_stdin_buffer == NULL) {
                perror("realloc");
                free(stdin_buffer);
                exit(EXIT_FAILURE);
            }
            stdin_buffer = new_stdin_buffer;
        }
    }

    // 2. Get current clipboard content
    char *clipboard_buffer = malloc(INITIAL_BUFFER_SIZE);
    if (clipboard_buffer == NULL) {
        perror("malloc");
        free(stdin_buffer);
        exit(EXIT_FAILURE);
    }
    size_t clipboard_capacity = INITIAL_BUFFER_SIZE;
    size_t clipboard_size = 0;

    FILE *clipboard_process_read = popen(paste_cmd, "r");
    if (clipboard_process_read == NULL) {
        if (errno == ENOENT) {
            fprintf(stderr, "Error: Command not found. Is '%s' installed and in your PATH?\n", paste_cmd);
        } else {
            perror("popen");
        }
        free(stdin_buffer);
        free(clipboard_buffer);
        exit(EXIT_FAILURE);
    }
    while ((n = fread(clipboard_buffer + clipboard_size, 1, clipboard_capacity - clipboard_size, clipboard_process_read)) > 0) {
        clipboard_size += n;
        if (clipboard_size == clipboard_capacity) {
            clipboard_capacity *= 2;
            char *new_clipboard_buffer = realloc(clipboard_buffer, clipboard_capacity);
            if (new_clipboard_buffer == NULL) {
                perror("realloc");
                free(stdin_buffer);
                free(clipboard_buffer);
                exit(EXIT_FAILURE);
            }
            clipboard_buffer = new_clipboard_buffer;
        }
    }
    pclose(clipboard_process_read);

    // 3. Send stdin content to clipboard
    if (stdin_size > 0) {
        FILE *clipboard_process_write = popen(copy_cmd, "w");
        if (clipboard_process_write == NULL) {
            if (errno == ENOENT) {
                fprintf(stderr, "Error: Command not found. Is '%s' installed and in your PATH?\n", copy_cmd);
            } else {
                perror("popen");
            }
            free(stdin_buffer);
            free(clipboard_buffer);
            exit(EXIT_FAILURE);
        }
        fwrite(stdin_buffer, 1, stdin_size, clipboard_process_write);
        pclose(clipboard_process_write);

        add_to_history(stdin_buffer, stdin_size);
    }

    // 4. Print original clipboard content to stdout
    if (clipboard_size > 0) {
        fwrite(clipboard_buffer, 1, clipboard_size, stdout);
    }

    free(stdin_buffer);
    free(clipboard_buffer);
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
        fprintf(stderr, "Usage: %s --send|--recv|--show|--exchange\n", argv[0]);
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
    } else if (strcmp(argv[1], "--exchange") == 0) {
        exchange_clipboard_and_stdin(copy_cmd, paste_cmd);
    } else {
        fprintf(stderr, "Usage: %s --send|--recv|--show|--exchange\n", argv[0]);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
