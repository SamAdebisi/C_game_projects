#ifndef EDUQ_CHALLENGE_H
#define EDUQ_CHALLENGE_H
#include <stddef.h>

typedef enum { SIG_SUM_ARRAY = 1 } ChallengeSig;
typedef int (*fn_sum_array)(const int *a, size_t n);

typedef struct { const int *input; size_t n; int expected; const char *hint; } SumArrayCase;

typedef struct Challenge {
    int id;
    const char *slug;
    const char *name;
    const char *description;
    ChallengeSig sig;
    void *solution_fn;
    const SumArrayCase *cases;
    size_t case_count;
    int xp_reward;
    int visibility;
} Challenge;

typedef struct { int passed, total; } GradeResult;

void challenges_init(void);
int  challenges_register(const Challenge *c);
int  challenges_count(void);
const Challenge* challenges_get(int idx);
GradeResult challenges_grade(const Challenge *c, int visibility);
#endif