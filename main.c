#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#include "cfg.h"
#include "generator.h"

char *sim_answer = NULL;
static bool is_letter_present[26] = { 0 };
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
    bool good_guess = false;
    for (int i = 0; i < 26; i++) {
        char letter = i + 'a';
        if (is_letter_present[i]) {
            good_guess = false;
            for (int j = 0; j < WORDLE_LENGTH; j++) {
                if (letter == guess[j]) {
                    good_guess = true;
                }
            }
            if (!good_guess) {
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
                printf("Guess: %s? (y/n)\n", guess);
                char yn = 'n';
                scanf("%c%*c", &yn);
                if (yn == 'y') {
                    break;
                } else {
                    have_guess = false;
                }
            } else {
                have_guess = false;
            }
        }
    }
    return have_guess;
}

void update_answer_map(char *guess, char *response, uint32_t* answer_map) {
    bool repeat[WORDLE_LENGTH] = { false };
    for (unsigned i = 0; i < WORDLE_LENGTH; i++) {
        for (unsigned j = 0; j < WORDLE_LENGTH; j++) {
            if (i != j) {
                if (guess[i] == guess[j]) {
                    repeat[i] = true;
                    repeat[j] = true;
                }
            }
        }
    }
    for (unsigned i = 0; i < WORDLE_LENGTH; i++) {
        switch (response[i]) {
        case 'g':
            // The letter is in this location
            // No need to keep track since all guesses will require this letter in this location
            answer_map[i] = char_to_bitmap(guess[i]);
            break;
        case 'y':
            // The letter is not in this location
            answer_map[i] &= ~char_to_bitmap(guess[i]);
            // But keep track of it so future guesses will contain it
            is_letter_present[guess[i] - 'a'] = true;
            break;
        case 'b':
            // If this is a repeat, it will counted as b, so ignore it. It has already been handled
            if (!repeat[i]) {
                // This letter is not in the word, clear its bit in all locations
                for (unsigned j = 0; j < WORDLE_LENGTH; j++) {
                    answer_map[j] &= ~char_to_bitmap(guess[i]);
                }
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
    bool repeat[WORDLE_LENGTH] = { false };
    for (unsigned i = 0; i < WORDLE_LENGTH - 1; i++) {
        for (unsigned j = i + 1; j < WORDLE_LENGTH; j++) {
            if (i != j) {
                if (guess[i] == guess[j]) {
                    repeat[j] = true;
                }
            }
        }
    }
    for (unsigned i = 0; i < WORDLE_LENGTH; i++) {
        response[i] = 'b';
        if (guess[i] == sim_answer[i]) {
            response[i] = 'g';
        } else {
            for (unsigned j = 0; j < WORDLE_LENGTH; j++) {
                if (guess[i] == sim_answer[j] && !repeat[i]) {
                    response[i] = 'y';
                    break;
                }
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
        } else {
            printf("Enter response:\n");
            scanf("%s", response);
        }
        printf("Response: %s\n", response);
        solved = true;
        for (unsigned i = 0; i < WORDLE_LENGTH; i++) {
            if (response[i] != 'g') {
                solved = false;
                break;
            }
        }
        update_answer_map(guess, response, answer_map);
    }
    printf("Done! Took %d rounds\n", rounds);
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
    printf("Using list of %u %u-letter words\n", num_words, WORDLE_LENGTH);

    bool is_sim = sim_answer != NULL;
    solve(words, num_words, is_sim);

    return 0;
}
