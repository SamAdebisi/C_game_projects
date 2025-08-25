// path: src/word-scramble-game.c
// Build: cc -std=c11 -O2 -Wall -Wextra -Wpedantic -o word_scramble src/word-scramble-game.c
// Run:   ./word_scramble
// Notes:
//  - Score = remaining tries when you guess correctly.
//  - Timer shows total elapsed seconds for the round.
//  - Type "quit" to exit at any prompt.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#define MAX_WORD_LEN 64
#define MAX_TRIES 6

static const char *WORDS[] = {
    "computer", "puzzle", "language", "pointer", "compiler",
    "variable", "function", "algorithm", "structure", "network",
    "concurrency", "optimize", "integer", "buffer", "security",
    "portable", "library", "recursion", "dynamic", "storage"
};
static const size_t WORDS_COUNT = sizeof(WORDS) / sizeof(WORDS[0]);

// Trims trailing newline from fgets
static void chomp(char *s) {
    size_t n = strlen(s);
    if (n && s[n - 1] == '\n') s[n - 1] = '\0';
}

// Case-insensitive strcmp for ASCII
static int ci_strcmp(const char *a, const char *b) {
    unsigned char ca, cb;
    while (*a && *b) {
        ca = (unsigned char)tolower((unsigned char)*a++);
        cb = (unsigned char)tolower((unsigned char)*b++);
        if (ca != cb) return (int)ca - (int)cb;
    }
    return (int)(unsigned char)tolower((unsigned char)*a) -
           (int)(unsigned char)tolower((unsigned char)*b);
}

// Fisher-Yates shuffle
static void shuffle_in_place(char *s, size_t len) {
    if (len < 2) return;
    for (size_t i = len - 1; i > 0; --i) {
        size_t j = (size_t)(rand() % (int)(i + 1));
        char tmp = s[i];
        s[i] = s[j];
        s[j] = tmp;
    }
}

static int is_all_same_chars(const char *s) {
    if (!s[0]) return 1;
    for (size_t i = 1; s[i]; ++i) if (s[i] != s[0]) return 0;
    return 1;
}

// Ensure scramble differs from original; fallback to rotation if needed
static void scramble_word(const char *src, char *dst, size_t dst_sz) {
    size_t len = strlen(src);
    if (len + 1 > dst_sz) {
        // defensive: truncate copy if oversized
        len = dst_sz - 1;
    }
    memcpy(dst, src, len);
    dst[len] = '\0';

    if (len < 2 || is_all_same_chars(src)) return; // nothing to do

    // Try a few shuffles to get a different sequence
    for (int attempt = 0; attempt < 10; ++attempt) {
        shuffle_in_place(dst, len);
        if (strcmp(dst, src) != 0) return;
    }
    // Fallback: rotate by one
    char first = dst[0];
    memmove(dst, dst + 1, len - 1);
    dst[len - 1] = first;
}

static int read_line(char *buf, size_t n) {
    if (!fgets(buf, (int)n, stdin)) return 0; // EOF
    chomp(buf);
    return 1;
}

static void print_hint(const char *word) {
    size_t len = strlen(word);
    char first = word[0];
    char last = word[len ? len - 1 : 0];
    printf("Hint: first='%c', last='%c', length=%zu\n", first, last, len);
}

static void play_round(void) {
    const char *target = WORDS[rand() % WORDS_COUNT];

    char scrambled[MAX_WORD_LEN];
    scramble_word(target, scrambled, sizeof(scrambled));

    int tries = MAX_TRIES;
    char guess[MAX_WORD_LEN];

    time_t t0 = time(NULL);

    printf("Scrambled: %s\n", scrambled);

    while (tries > 0) {
        printf("Tries left: %d\n", tries);
        printf("Your guess: ");
        fflush(stdout);

        if (!read_line(guess, sizeof(guess))) {
            printf("Input ended.\n");
            return;
        }
        if (ci_strcmp(guess, "quit") == 0) {
            printf("Quit.\n");
            return;
        }
        if (guess[0] == '\0') { // ignore empty
            printf("Empty input ignored.\n");
            continue;
        }

        if (ci_strcmp(guess, target) == 0) {
            time_t t1 = time(NULL);
            long elapsed = (long)difftime(t1, t0);
            printf("Correct. Word=\"%s\". Score=%d. Time=%lds.\n", target, tries, elapsed);
            return;
        }

        // wrong
        --tries;
        print_hint(target); // show hint and penalize via tries-- already done
        if (tries == 0) {
            time_t t1 = time(NULL);
            long elapsed = (long)difftime(t1, t0);
            printf("Out of tries. The word was: %s. Score=0. Time=%lds.\n", target, elapsed);
            return;
        }
    }
}

int main(void) {
    // Seed RNG using time
    srand((unsigned int)time(NULL));

    printf("Word Scramble — type 'quit' to exit.\n\n");

    for (;;) {
        play_round();
        printf("\nPlay again? (y/n): ");
        char line[16];
        if (!read_line(line, sizeof(line))) break;
        if (line[0] != 'y' && line[0] != 'Y') break;
        printf("\n");
    }

    printf("Bye.\n");
    return 0;
}
