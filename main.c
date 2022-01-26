#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

#define WORDLE_LENGTH           (5)
#define MAX_NUM_WORDS           (20000)
const char binary_filename[] = "5_letter_words.bin";

char *generate(char *word_list_filename, unsigned *num_words) {
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
                }
                tmp_word[i] = c;
            } else {
                if (c == '\n') {
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
    fwrite(word_buffer, sizeof(char), *num_words, binary_f);
    fclose(binary_f);
    return word_buffer;
}

void solve(char *words, unsigned num_words) {

}

void usage(void) {
    fprintf(stderr, "Usage: ./solver <subcommand> [subcommand arg] [file]\n");
    fprintf(stderr, "Subcommands are:\n");
    fprintf(stderr, "   solve - Gives guesses for solving a real puzzle\n");
    fprintf(stderr, "   sim - Simulates guesses given a word to solve for\n");
    fprintf(stderr, "Filename needed to word list if generated binary list is not present\n");
}


int main(int argc, char **argv) {
    (void) argc;
    argv++;
    char *subcommand = *argv++;
    char *answer = NULL;
    if (subcommand == NULL) {
        fprintf(stderr, "Please provide a subcommand\n");
        usage();
        exit(1);
    }
    if (!strcmp(subcommand, "sim")) {
        answer = *argv++;
        if (answer == NULL) {
            fprintf(stderr, "Please provide an answer word for the sim\n");
            usage();
            exit(1);
        }
    }

    FILE *f = fopen(binary_filename, "rb");
    char *words = NULL;
    unsigned num_words = 0;
    if (f == NULL) {
        printf("Binary list not found, generating from text file...\n");
        words = generate(*argv, &num_words);
        if (num_words == 0 || words == NULL) {
            fprintf(stderr, "Error in reading in words\n");
            exit(1);
        }
    } else {
        printf("Binary list found, using it\n");
        fread(&num_words, sizeof(unsigned), 1, f);
        words = (char*) malloc (sizeof(char) * num_words * WORDLE_LENGTH);
        fread(words, sizeof(char), num_words, f);
        fclose(f);
    }
    printf("Using list of %u %u-letter words\n", num_words, WORDLE_LENGTH);

    if (answer == NULL) {
        solve();
    } else {
        sim();
    }

    return 0;
}
