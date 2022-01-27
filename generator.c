#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "cfg.h"

void sort_by_letter_frequency(char *buffer, unsigned num_words) {
    char *original_buffer = buffer;
    float num_words_f = (float) num_words;
    // 1. Count frequency of letter appearance in each location, weighted by word frequency
    float letter_position_frequency[WORDLE_LENGTH][26] = {{0.0}};
    for (unsigned i = 0; i < num_words; i++, buffer += WORDLE_LENGTH) {
        float weight = 1.0f - ((float) i / num_words_f);
        for (unsigned j = 0; j < WORDLE_LENGTH; j++) {
            letter_position_frequency[j][buffer[j] - 'a'] += weight;
        }
    }
    printf("\n============================================================================================================\n");
    printf("Frequency of each letter in each position in word. Weighted for word frequency (if the input list is sorted)\n");
    printf("Position  ");
    for (int i = 0; i < 26; i++) {
        printf("   %c  ", (char)('a' + i));
    }
    printf("\n");
    for (int i = 0; i < WORDLE_LENGTH; i++) {
        printf("%8u ", i);
        for (int j = 0; j < 26; j++) {
            printf("%5.1f ", letter_position_frequency[i][j]);
        }
        printf("\n");
    }
    printf("\n============================================================================================================\n\n");
    // 2. Score each word in list for letter frequency
    float *scores = (float*) malloc(sizeof(float) * num_words);
    buffer = original_buffer;
    for (unsigned i = 0; i < num_words; i++, buffer += WORDLE_LENGTH) {
        for (unsigned j = 0; j < WORDLE_LENGTH; j++) {
            scores[i] += letter_position_frequency[j][buffer[j] - 'a'];
        }
    }
    // 3. Sort by scores
    char *buffer_copy = (char*) malloc(sizeof(char) * num_words * WORDLE_LENGTH);
    memcpy(buffer_copy, original_buffer, sizeof(char) * num_words * WORDLE_LENGTH);
    unsigned *score_indicies = (unsigned*) malloc(sizeof(unsigned) * num_words);
    for (unsigned i = 0; i < num_words; i++) {
        score_indicies[i] = i;
    }
    for (int i = 1; i < num_words; i++) {
        unsigned tmp = score_indicies[i];
        float key = scores[i];
        int j = i - 1;

        while (j >= 0 && scores[j] < key) {
            scores[j + 1] = scores[j];
            score_indicies[j + 1] = score_indicies[j];
            j--;
        }
        scores[j + 1] = key;
        score_indicies[j + 1] = tmp;
    }
    buffer = original_buffer;
    for (unsigned i = 0; i < num_words; i++, buffer += WORDLE_LENGTH) {
        unsigned src_index = score_indicies[i];
        memcpy(buffer, buffer_copy + (src_index * WORDLE_LENGTH), WORDLE_LENGTH * sizeof(char));
    }
    free(scores);
    free(score_indicies);
    free(buffer_copy);
}

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
