// ============================
// file: src/core.h
// ============================
#ifndef CORE_H
#define CORE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#ifdef _WIN32
  #include <direct.h>
  #include <windows.h>
  #define PATH_SEP '\\'
#else
  #include <sys/stat.h>
  #include <sys/types.h>
  #include <unistd.h>
  #define PATH_SEP '/'
#endif

#define EDUQ_VERSION "0.1.0"
#define EDUQ_APPNAME "EduQuest"

// Minimal log macro. Avoids noisy output in grader paths.
#define LOG(fmt, ...) do { fprintf(stderr, "[eduq] " fmt "\n", ##__VA_ARGS__); } while(0)

// ----------------------------
// Profile types
// ----------------------------
typedef struct {
    char  name[64];
    int   xp;
    int   level;
    int   challenges_solved;
} Profile;

static inline int xp_to_level(int xp) {
    // Why: simple predictable curve for early prototyping
    return xp / 100 + 1;
}

// ----------------------------
// Event bus
// ----------------------------
typedef enum {
    EV_NONE = 0,
    EV_XP_GAIN,
    EV_CHALLENGE_PASSED,
    EV_SAVED,
} EventType;

typedef struct {
    EventType type;
    int       i1;     // generic int payload
    const char *s1;   // generic string payload
} Event;

typedef void (*EventHandler)(const Event *ev, void *user);

#define MAX_SUBS 64

typedef struct {
    EventHandler handlers[MAX_SUBS];
    void*        user[MAX_SUBS];
    size_t       count;
} EventBus;

void eventbus_init(EventBus *bus);
bool eventbus_subscribe(EventBus *bus, EventHandler h, void *user);
void eventbus_publish(EventBus *bus, const Event *ev);

// ----------------------------
// Save + analytics interfaces
// ----------------------------
char *get_user_dir(char *buf, size_t bufsz);
char *get_save_dir(char *buf, size_t bufsz);
char *get_save_path(char *buf, size_t bufsz);
char *get_analytics_path(char *buf, size_t bufsz);

bool save_profile(const Profile *p);
bool load_profile(Profile *p);

void analytics_log_header_if_needed(void);
void analytics_log_event(const char *kind, const char *detail, int v);

// ----------------------------
// Challenge engine interfaces
// ----------------------------
typedef enum {
    SIG_SUM_ARRAY = 1,
    // future: SIG_STRING_TRANSFORM, SIG_RECURSION_INT, ...
} ChallengeSig;

typedef int (*fn_sum_array)(const int *a, size_t n);

typedef struct {
    // Why: narrow test case struct keeps grader simple
    const int *input;
    size_t n;
    int expected;
    const char *hint;
} SumArrayCase;

typedef struct Challenge Challenge;

struct Challenge {
    int id;
    const char *slug;
    const char *name;
    const char *description;
    ChallengeSig sig;
    void *solution_fn;   // cast per sig

    // tests for SIG_SUM_ARRAY
    const SumArrayCase *cases;
    size_t case_count;

    int xp_reward;
    int visibility; // 0 hidden hints, 1 basic hints, 2 full
};

typedef struct {
    int passed;
    int total;
} GradeResult;

void challenges_init(void);
int  challenges_register(const Challenge *c);
int  challenges_count(void);
const Challenge* challenges_get(int idx);
GradeResult challenges_grade(const Challenge *c, int visibility);

// content packs
void register_content_pack_arrays(void);

// Utility
static inline void clamp_line(char *s) {
    if (!s) return;
    size_t n = strlen(s);
    while (n && (s[n-1] == '\n' || s[n-1] == '\r')) { s[--n] = '\0'; }
}

#endif // CORE_H

