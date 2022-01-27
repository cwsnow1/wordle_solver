#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#include "cfg.h"
#include "generator.h"

char *sim_answer = NULL;
static unsigned known_letter_count[26] = { 0 };
const char binary_filename[] = "5_letter_words.bin";


uint32_t char_to_bitmap(char c) {
    if (c < 'a') {
        return 0;
    }
    return (1 << (c - 'a'));
}

bool is_letter_valid(uint32_t answer_map, char c) {
    return answer_map & char_to_bitmap(c);
}

///! @brief Check that the guess contains all the letters that are known to be in the word
bool is_a_good_guess(char *guess) {
    for (int i = 0; i < 26; i++) {
        char letter = i + 'a';
        if (known_letter_count[i]) {
            unsigned letter_count = 0;
            for (int j = 0; j < WORDLE_LENGTH; j++) {
                if (letter == guess[j]) {
                    letter_count++;
                }
            }
            if (letter_count < known_letter_count[i]) {
                return false;
            }
        }
    }
    return true;
}

bool guess_word(char **words, unsigned *num_words, uint32_t *answer_map, char *guess) {
    bool have_guess;
    unsigned original_num_words = *num_words;
    for (unsigned i = 0; i < original_num_words; i++, *words += WORDLE_LENGTH) {
        have_guess = true;
        for (unsigned j = 0; j < WORDLE_LENGTH; j++) {
            if (!is_letter_valid(answer_map[j], (*words)[j])) {
                have_guess = false;
                *num_words -= 1;
                break;
            }
        }
        if (have_guess) {
            // We have a word that isn't technically wrong
            memcpy(guess, *words, sizeof(char) * WORDLE_LENGTH);
            if (is_a_good_guess(guess)) {
                printf("Guess: %s\n", guess);
                break;
            } else {
                have_guess = false;
            }
        }
    }
    return have_guess;
}

void update_answer_map(char *guess, char *response, uint32_t* answer_map) {
    bool repeat[WORDLE_LENGTH] = { false };
    for (unsigned i = 0; i < WORDLE_LENGTH - 1; i++) {
        for (unsigned j = i + 1; j < WORDLE_LENGTH; j++) {
            if (guess[i] == guess[j]) {
                repeat[j] = true;
            }
        }
    }
    memset(known_letter_count, 0, 26 * sizeof(unsigned));
    for (unsigned i = 0; i < WORDLE_LENGTH; i++) {
        switch (response[i]) {
        case 'g':
        case 'G':
            // The letter is in this location
            answer_map[i] = char_to_bitmap(guess[i]);
            known_letter_count[guess[i] - 'a']++;
            break;
        case 'y':
        case 'Y':
            // The letter is not in this location
            answer_map[i] &= ~char_to_bitmap(guess[i]);
            // But keep track of it so future guesses will contain it
            known_letter_count[guess[i] - 'a']++;
            break;
        case 'b':
        case 'B':
            // If this is a repeat, it will counted as b, so only invalidate this spot. It has already been handled
            if (!repeat[i]) {
                // This letter is not in the word, clear its bit in all locations
                for (unsigned j = 0; j < WORDLE_LENGTH; j++) {
                    answer_map[j] &= ~char_to_bitmap(guess[i]);
                }
            } else {
                answer_map[i] &= ~char_to_bitmap(guess[i]);
            }
            break;
        default:
            fprintf(stderr, "Invalid response input\n");
            exit(1);
            break;
        }
    }
}

void get_sim_response(char *guess, char *response) {
    int real_letter_counts[26] = { 0 };
    for (unsigned i = 0; i < WORDLE_LENGTH; i++) {
        real_letter_counts[sim_answer[i] - 'a']++;
    }
    // Need 2 sweeps
    // First get all the matches and decrement counters
    // so that repeat letters that appear earlier in the
    // word are not counted as yellow when they should be black
    for (unsigned i = 0; i < WORDLE_LENGTH; i++) {
        response[i] = 'b';
        if (guess[i] == sim_answer[i]) {
            response[i] = 'g';
            real_letter_counts[guess[i] - 'a']--;
        }
    }
    for (unsigned i = 0; i < WORDLE_LENGTH; i++) {
        if (guess[i] != sim_answer[i]) {
            if (real_letter_counts[guess[i] - 'a']) {
                response[i] = 'y';
                real_letter_counts[guess[i] - 'a']--;
            }
        }
    }
}

void solve(char *words, unsigned num_words, bool is_sim) {
    printf("A guess will be provided for each round and the response will need to be input\n");
    printf("Reponse is in this format:\n");
    printf("g - correct letter, correct position\n");
    printf("y - correct letter, wrong position\n");
    printf("b - wrong letter\n");
    memset(known_letter_count, 0, 26 * sizeof(unsigned));
    uint32_t answer_map[WORDLE_LENGTH];
    for (unsigned i = 0; i < WORDLE_LENGTH; i++) {
        answer_map[i] = (1 << 26) - 1;
    }
    bool solved = false;
    bool have_guess = true;
    int rounds = 0;
    while (!solved) {
        rounds++;
        char guess[WORDLE_LENGTH + 1];
        guess[WORDLE_LENGTH] = '\0';
        have_guess = guess_word(&words, &num_words, answer_map, guess);
        if (!have_guess) {
            printf("No guesses found in word list... sorry\n");
            return;
        }
        char response[WORDLE_LENGTH + 1];
        response[WORDLE_LENGTH] = '\0';
        if (is_sim) {
            get_sim_response(guess, response);
            printf("Response: %s\n", response);
        } else {
            printf("Enter response:\n");
            scanf("%s%*c", response);
        }
        solved = true;
        for (unsigned i = 0; i < WORDLE_LENGTH; i++) {
            if (response[i] != 'g') {
                solved = false;
                break;
            }
        }
        update_answer_map(guess, response, answer_map);
    }
    printf("Done! Took %d rounds\n\n", rounds);
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
    if (subcommand == NULL) {
        fprintf(stderr, "Please provide a subcommand\n");
        usage();
        exit(1);
    }
    if (!strcmp(subcommand, "sim")) {
        sim_answer = *argv++;
        if (sim_answer == NULL) {
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
        words = generate(*argv, &num_words, binary_filename);
        if (num_words == 0 || words == NULL) {
            fprintf(stderr, "Error in reading in words\n");
            exit(1);
        }
    } else {
        printf("Binary list found, using it\n");
        fread(&num_words, sizeof(unsigned), 1, f);
        words = (char*) malloc (sizeof(char) * num_words * WORDLE_LENGTH);
        fread(words, sizeof(char), num_words * WORDLE_LENGTH, f);
        fclose(f);
    }
    char *words_sorted_by_letter_frequency = (char*) malloc(sizeof(char) * num_words * WORDLE_LENGTH);
    memcpy(words_sorted_by_letter_frequency, words, sizeof(char) * num_words * WORDLE_LENGTH);
    sort_by_letter_frequency(words_sorted_by_letter_frequency, num_words);

    bool is_sim = sim_answer != NULL;
    printf("Using list of %u %u-letter words, sorted by usage\n", num_words, WORDLE_LENGTH);
    solve(words, num_words, is_sim);

    printf("Using list of %u %u-letter words, sorted by letter-position frequency (weighted for usage)\n", num_words, WORDLE_LENGTH);
    solve(words_sorted_by_letter_frequency, num_words, is_sim);
    free(words);

    return 0;
}
