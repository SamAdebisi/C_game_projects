// # Pseudocode
// - Define defaults: MIN=1, MAX=100, ATTEMPTS=10
// - Read optional CLI args: [min max attempts]; validate order and ranges
// - Seed RNG with current time
// - Pick target = random integer in [min, max]
// - For attempt from 1..attempts
//   - Prompt: "Guess (min-max), attempt i of attempts: "
//   - Read line -> parse to long with strtol; if invalid or out of range, 
//   show error and do not consume attempt, continue
//   - If guess == target -> print "Win" with attempts used, exit success
//   - Else print hint: "Higher" if guess < target else "Lower"
// - If loop ends -> print "Lose" and reveal target; exit with code 1

// file: src/main.c
// build: cc -std=c99 -O2 -Wall -Wextra -pedantic -o guess src/main.c
// why-comments: Only explain non-obvious choices.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <errno.h>

#define DEFAULT_MIN 1
#define DEFAULT_MAX 100
#define DEFAULT_ATTEMPTS 10

static void usage(const char *prog) {
    fprintf(stderr,
            "Usage: %s [min max attempts]\n"
            "Defaults: min=%d max=%d attempts=%d\n",
            prog, DEFAULT_MIN, DEFAULT_MAX, DEFAULT_ATTEMPTS);
}

static int parse_long(const char *s, long *out) {
    // Strtol with robust error checks.
    if (!s || !*s) return 0;
    char *end = NULL;
    errno = 0;
    long v = strtol(s, &end, 10);
    if (errno != 0) return 0;
    if (end == s || *end != '\0') return 0;
    *out = v;
    return 1;
}

static int read_guess(long min, long max, long *guess_out) {
    // Reads a line and validates bounds. Returns 1 if valid guess, 0 otherwise.
    char buf[128];
    if (!fgets(buf, sizeof buf, stdin)) return 0; // EOF
    // Strip trailing newline if present.
    size_t len = strlen(buf);
    if (len && buf[len - 1] == '\n') buf[len - 1] = '\0';

    long g;
    if (!parse_long(buf, &g)) {
        printf("Invalid input. Enter an integer.\n");
        return 0;
    }
    if (g < min || g > max) {
        printf("Out of range. Enter a value between %ld and %ld.\n", min, max);
        return 0;
    }
    *guess_out = g;
    return 1;
}

static long rand_inclusive(long min, long max) {
    // Avoid modulo bias using 64-bit range mapping when possible.
    unsigned long range = (unsigned long)(max - min + 1);
    unsigned long limit = (ULONG_MAX / range) * range; // largest multiple <= ULONG_MAX
    unsigned long r;
    do {
        // Compose a wide random from two rand() calls to improve entropy portability.
        unsigned long r1 = (unsigned long)rand();
        unsigned long r2 = (unsigned long)rand();
        r = (r1 << (sizeof(int) * 4)) ^ r2; // mix bits; keeps it simple and portable
    } while (r >= limit);
    return (long)(min + (r % range));
}

int main(int argc, char **argv) {
    long min = DEFAULT_MIN;
    long max = DEFAULT_MAX;
    long attempts = DEFAULT_ATTEMPTS;

    if (argc == 2 && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))) {
        usage(argv[0]);
        return 0;
    }

    if (argc >= 2) {
        if (argc < 4 || argc > 4) {
            usage(argv[0]);
            return 2;
        }
        long tmin, tmax, tatt;
        if (!parse_long(argv[1], &tmin) || !parse_long(argv[2], &tmax) || !parse_long(argv[3], &tatt)) {
            fprintf(stderr, "Argument parsing failed.\n");
            return 2;
        }
        if (tmin >= tmax) {
            fprintf(stderr, "min must be < max.\n");
            return 2;
        }
        if (tatt <= 0 || tatt > 100000) { // guard absurd values
            fprintf(stderr, "attempts must be in 1..100000.\n");
            return 2;
        }
        min = tmin; max = tmax; attempts = tatt;
    }

    // Seed once per process using high-resolution time.
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    unsigned int seed = (unsigned int)(ts.tv_nsec ^ ts.tv_sec);
    srand(seed);

    long target = rand_inclusive(min, max);

    printf("Target picked. Range [%ld..%ld]. Attempts: %ld.\n", min, max, attempts);

    long remaining = attempts;
    for (long turn = 1; turn <= attempts; ++turn) {
        printf("Guess (%ld-%ld) attempt %ld of %ld: ", min, max, turn, attempts);
        fflush(stdout);

        long guess;
        if (!read_guess(min, max, &guess)) {
            // Do not consume attempt on invalid input.
            --turn;
            continue;
        }

        if (guess == target) {
            printf("Win: %ld is correct. Used %ld/%ld attempts.\n", target, turn, attempts);
            return 0;
        }
        if (guess < target) {
            printf("Higher.\n");
        } else {
            printf("Lower.\n");
        }
        remaining = attempts - turn;
        if (remaining > 0) printf("%ld attempt%s left.\n", remaining, remaining == 1 ? "" : "s");
    }

    printf("Lose: out of attempts. Target was %ld.\n", target);
    return 1;
}
