#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "cfg.h"

char *generate(char *word_list_filename, unsigned *num_words, const char *binary_filename) {
    if (word_list_filename == NULL) {
        fprintf(stderr, "Please provide a word list as an argument\n");
        exit(1);
    }
    FILE *word_list_f = fopen(word_list_filename, "r");
    if (word_list_f == NULL) {
        fprintf(stderr, "Error reading %s: %s\n", word_list_filename, strerror(errno));
        exit(1);
    }
    char *word_buffer = (char*) malloc(sizeof(char) * MAX_NUM_WORDS * WORDLE_LENGTH);
    char *word_buffer_ptr = word_buffer;
    bool done = false;
    *num_words = 0;
    while (!done) {
        char tmp_word[WORDLE_LENGTH];
        for (int i = 0; i < WORDLE_LENGTH + 1; i++) {
            char c = (char) getc(word_list_f);
            if (c == EOF) {
                done = true;
                break;
            }
            if (i < WORDLE_LENGTH) {
                if (c == '\n') {
                    break;
                } else if (c == '\r') {
                    while (c != '\n') {
                        c = (char) getc(word_list_f);
                    }
                    break;
                }
                tmp_word[i] = c;
            } else {
                if (c == '\n' || c == '\r') {
                    memcpy(word_buffer_ptr, tmp_word, sizeof(char) * WORDLE_LENGTH);
                    word_buffer_ptr += WORDLE_LENGTH;
                    (*num_words)++;
                } else {
                    while ((c != '\n') && (c != EOF)) {
                        c = (char) getc(word_list_f);
                    }
                }
            }
        }
    }
    fclose(word_list_f);
    FILE *binary_f = fopen(binary_filename, "wb");
    fwrite(num_words, sizeof(unsigned), 1, binary_f);
    fwrite(word_buffer, sizeof(char), * num_words * WORDLE_LENGTH, binary_f);
    fclose(binary_f);
    return word_buffer;
}
