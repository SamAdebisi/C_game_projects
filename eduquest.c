// ============================
// file: README.build
// ============================
// Build (POSIX: Linux/macOS):
//   cc -std=c17 -Wall -Wextra -O2 -o eduquest \
//      src/*.c player/*.c
//
// Build (Windows, MSVC):
//   cl /std:c17 /W4 /O2 /Fe:eduquest.exe src\*.c player\*.c
//
// Run:
//   ./eduquest
//
// Notes:
// - Edit player/player_solutions.c to implement solutions.
// - Save files live in OS-appropriate user folders.
// - Analytics logs to analytics.csv next to the save file.

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

// ============================
// file: src/event_bus.c
// ============================
#include "src/core.h"

void eventbus_init(EventBus *bus) {
    bus->count = 0;
}

bool eventbus_subscribe(EventBus *bus, EventHandler h, void *user) {
    if (bus->count >= MAX_SUBS) return false;
    bus->handlers[bus->count] = h;
    bus->user[bus->count] = user;
    bus->count++;
    return true;
}

void eventbus_publish(EventBus *bus, const Event *ev) {
    for (size_t i = 0; i < bus->count; ++i) {
        bus->handlers[i](ev, bus->user[i]);
    }
}

// ============================
// file: src/save.c
// ============================
#include "src/core.h"

static void ensure_dir(const char *path) {
#ifdef _WIN32
    _mkdir(path);
#else
    mkdir(path, 0755);
#endif
}

char *get_user_dir(char *buf, size_t bufsz) {
#ifdef _WIN32
    const char *appdata = getenv("APPDATA");
    if (!appdata) appdata = ".";
    snprintf(buf, bufsz, "%s", appdata);
#else
    const char *home = getenv("HOME");
    if (!home) home = ".";
    snprintf(buf, bufsz, "%s", home);
#endif
    return buf;
}

char *get_save_dir(char *buf, size_t bufsz) {
    char base[512];
    get_user_dir(base, sizeof base);
#ifdef _WIN32
    snprintf(buf, bufsz, "%s% c%s", base, PATH_SEP, EDUQ_APPNAME);
#else
    snprintf(buf, bufsz, "%s% c.%s", base, PATH_SEP, EDUQ_APPNAME);
#endif
    // fix accidental space after % if typo
    for (char *p = buf; *p; ++p) if (*p == ' ') *p = PATH_SEP;
    ensure_dir(buf);
    return buf;
}

char *get_save_path(char *buf, size_t bufsz) {
    char dir[512];
    get_save_dir(dir, sizeof dir);
    snprintf(buf, bufsz, "%s%cprofile.txt", dir, PATH_SEP);
    return buf;
}

char *get_analytics_path(char *buf, size_t bufsz) {
    char dir[512];
    get_save_dir(dir, sizeof dir);
    snprintf(buf, bufsz, "%s%canalytics.csv", dir, PATH_SEP);
    return buf;
}

bool save_profile(const Profile *p) {
    char path[512];
    get_save_path(path, sizeof path);
    FILE *f = fopen(path, "w");
    if (!f) return false;
    fprintf(f, "name=%s\n", p->name);
    fprintf(f, "xp=%d\n", p->xp);
    fprintf(f, "level=%d\n", p->level);
    fprintf(f, "solved=%d\n", p->challenges_solved);
    fclose(f);
    Event ev = { .type = EV_SAVED };
    (void)ev; // bus is in main; kept simple here
    return true;
}

bool load_profile(Profile *p) {
    memset(p, 0, sizeof *p);
    strncpy(p->name, "Adventurer", sizeof p->name - 1);
    p->xp = 0; p->level = 1; p->challenges_solved = 0;

    char path[512];
    get_save_path(path, sizeof path);
    FILE *f = fopen(path, "r");
    if (!f) return true; // fresh profile

    char line[256];
    while (fgets(line, sizeof line, f)) {
        clamp_line(line);
        char key[64], val[128];
        if (sscanf(line, "%63[^=]=%127s", key, val) == 2) {
            if (strcmp(key, "name") == 0) {
                strncpy(p->name, val, sizeof p->name - 1);
            } else if (strcmp(key, "xp") == 0) {
                p->xp = atoi(val);
            } else if (strcmp(key, "level") == 0) {
                p->level = atoi(val);
            } else if (strcmp(key, "solved") == 0) {
                p->challenges_solved = atoi(val);
            }
        }
    }
    fclose(f);
    // sync derived
    p->level = xp_to_level(p->xp);
    return true;
}

// ============================
// file: src/analytics.c
// ============================
#include "src/core.h"

static int file_exists(const char *p) {
    FILE *f = fopen(p, "r");
    if (!f) return 0;
    fclose(f);
    return 1;
}

void analytics_log_header_if_needed(void) {
    char path[512];
    get_analytics_path(path, sizeof path);
    if (file_exists(path)) return;
    FILE *f = fopen(path, "w");
    if (!f) return;
    fprintf(f, "timestamp,kind,detail,value\n");
    fclose(f);
}

static void now_iso(char *buf, size_t n) {
    time_t t = time(NULL);
    struct tm tmv;
#ifdef _WIN32
    localtime_s(&tmv, &t);
#else
    localtime_r(&t, &tmv);
#endif
    strftime(buf, n, "%Y-%m-%dT%H:%M:%S", &tmv);
}

void analytics_log_event(const char *kind, const char *detail, int v) {
    char path[512];
    get_analytics_path(path, sizeof path);
    FILE *f = fopen(path, "a");
    if (!f) return;
    char ts[32]; now_iso(ts, sizeof ts);
    fprintf(f, "%s,%s,%s,%d\n", ts, kind ? kind : "", detail ? detail : "", v);
    fclose(f);
}

// ============================
// file: src/challenge.c
// ============================
#include "src/core.h"

#define MAX_CHALLENGES 64
static Challenge g_chals[MAX_CHALLENGES];
static int g_chal_count = 0;

void challenges_init(void) {
    g_chal_count = 0;
}

int challenges_register(const Challenge *c) {
    if (g_chal_count >= MAX_CHALLENGES) return -1;
    g_chals[g_chal_count] = *c;
    g_chals[g_chal_count].id = g_chal_count;
    return g_chal_count++;
}

int challenges_count(void) { return g_chal_count; }

const Challenge* challenges_get(int idx) {
    if (idx < 0 || idx >= g_chal_count) return NULL;
    return &g_chals[idx];
}

GradeResult challenges_grade(const Challenge *c, int visibility) {
    GradeResult r = {0, 0};
    if (!c) return r;

    if (c->sig == SIG_SUM_ARRAY) {
        fn_sum_array fn = (fn_sum_array)c->solution_fn;
        for (size_t i = 0; i < c->case_count; ++i) {
            const SumArrayCase *tc = &c->cases[i];
            int got = fn(tc->input, tc->n);
            r.total++;
            if (got == tc->expected) {
                r.passed++;
            } else {
                if (visibility > 0) {
                    printf("  • Case %zu failed: expected %d got %d\n", i+1, tc->expected, got);
                    if (visibility > 1 && tc->hint) printf("    hint: %s\n", tc->hint);
                }
            }
        }
    }
    return r;
}

// ============================
// file: src/content_arrays.c
// ============================
#include "src/core.h"

// Declaration of the learner function from player code
int sum_array(const int *a, size_t n);

static const int A1[] = {1,2,3,4,5};
static const int A2[] = { -2, 7, -1, 0 };
static const int A3[] = { };
static const int A4[] = { 1000000000, 1000000000, -1000000000 }; // overflow-safe test intent

static const SumArrayCase SUM_CASES[] = {
    { A1, 5, 15, "accumulate all elements" },
    { A2, 4, 4,  "handle negatives" },
    { A3, 0, 0,  "empty arrays sum to 0" },
    { A4, 3, 1000000000, "watch for overflow only if using wider type" },
};

void register_content_pack_arrays(void) {
    static Challenge sumc = {
        .slug = "arrays.sum",
        .name = "Sum of Array",
        .description = "Implement sum_array(const int*, size_t) to return the sum of elements.",
        .sig = SIG_SUM_ARRAY,
        .solution_fn = (void*)sum_array,
        .cases = SUM_CASES,
        .case_count = sizeof(SUM_CASES)/sizeof(SUM_CASES[0]),
        .xp_reward = 100,
        .visibility = 1,
    };
    challenges_register(&sumc);
}

// ============================
// file: player/player_solutions.c
// ============================
#include <stddef.h>
#include <stdint.h>

// Learner edits functions in this file.
int sum_array(const int *a, size_t n) {
    // Why: baseline correct implementation for first run
    long long acc = 0; // reduce UB risk in intermediate sum
    for (size_t i = 0; i < n; ++i) acc += a[i];
    // clamp back to int domain
    if (acc > INT32_MAX) acc = INT32_MAX;
    if (acc < INT32_MIN) acc = INT32_MIN;
    return (int)acc;
}

// ============================
// file: src/main.c
// ============================
#include "src/core.h"

static EventBus G_BUS;
static Profile  G_PROFILE;

static void on_event(const Event *ev, void *user) {
    (void)user;
    if (ev->type == EV_XP_GAIN) {
        analytics_log_event("xp_gain", ev->s1, ev->i1);
    } else if (ev->type == EV_CHALLENGE_PASSED) {
        analytics_log_event("challenge_pass", ev->s1, ev->i1);
    } else if (ev->type == EV_SAVED) {
        analytics_log_event("saved", "profile", 1);
    }
}

static void banner(void) {
    printf("\n== %s v%s ==\n", EDUQ_APPNAME, EDUQ_VERSION);
}

static void show_profile(void) {
    printf("\nProfile: %s | XP: %d | Level: %d | Solved: %d\n",
           G_PROFILE.name, G_PROFILE.xp, G_PROFILE.level, G_PROFILE.challenges_solved);
}

static void ensure_profile_named(void) {
    if (strcmp(G_PROFILE.name, "Adventurer") == 0) {
        printf("Enter profile name: ");
        char buf[64];
        if (fgets(buf, sizeof buf, stdin)) { clamp_line(buf); }
        if (buf[0]) strncpy(G_PROFILE.name, buf, sizeof G_PROFILE.name - 1);
    }
}

static void do_list_challenges(void) {
    int n = challenges_count();
    printf("\nQuests available (%d):\n", n);
    for (int i = 0; i < n; ++i) {
        const Challenge *c = challenges_get(i);
        printf("  [%d] %s — %s\n", i, c->name, c->slug);
    }
}

static const Challenge* select_challenge(void) {
    do_list_challenges();
    printf("Select quest id: ");
    int id = -1; if (scanf("%d", &id) != 1) { while (getchar()!='\n' && !feof(stdin)); return NULL; }
    while (getchar()!='\n' && !feof(stdin));
    return challenges_get(id);
}

static void enter_quest(void) {
    const Challenge *c = select_challenge();
    if (!c) { printf("Invalid selection.\n"); return; }
    printf("\nQuest: %s\n%s\n", c->name, c->description);
    printf("Run tests now? [y/N]: ");
    char ch = (char)getchar(); while (getchar()!='\n' && !feof(stdin));
    if (ch=='y' || ch=='Y') {
        GradeResult r = challenges_grade(c, c->visibility);
        printf("\nResult: %d/%d passed\n", r.passed, r.total);
        if (r.passed == r.total) {
            printf("Reward: +%d XP\n", c->xp_reward);
            G_PROFILE.xp += c->xp_reward;
            int lvl_before = G_PROFILE.level;
            G_PROFILE.level = xp_to_level(G_PROFILE.xp);
            G_PROFILE.challenges_solved += 1;
            Event ev1 = { .type = EV_XP_GAIN, .i1 = c->xp_reward, .s1 = c->slug };
            Event ev2 = { .type = EV_CHALLENGE_PASSED, .i1 = 1, .s1 = c->slug };
            eventbus_publish(&G_BUS, &ev1);
            eventbus_publish(&G_BUS, &ev2);
            if (G_PROFILE.level > lvl_before) {
                printf("Level up → %d\n", G_PROFILE.level);
            }
        } else {
            printf("Keep iterating. Edit player/player_solutions.c and rebuild.\n");
        }
    } else {
        printf("Use 'Enter Quest' again when ready.\n");
    }
}

static void save_now(void) {
    if (save_profile(&G_PROFILE)) {
        printf("Saved.\n");
        Event ev = { .type = EV_SAVED };
        eventbus_publish(&G_BUS, &ev);
    } else {
        printf("Save failed.\n");
    }
}

static void overworld(void) {
    printf("\n[Overworld] Zones: Arrays (1) | Recursion (locked) | OOP (locked)\n");
}

static void skill_tree(void) {
    printf("\n[Skill Tree]\n  Fundamentals %s\n  Arrays %s\n  Recursion %s\n",
           (G_PROFILE.level>=1? "✓": ""),
           (G_PROFILE.challenges_solved>=1? "✓": ""),
           (G_PROFILE.level>=3? "Unlocked": "Locked"));
}

int main(void) {
    analytics_log_header_if_needed();
    eventbus_init(&G_BUS);
    eventbus_subscribe(&G_BUS, on_event, NULL);

    load_profile(&G_PROFILE);
    ensure_profile_named();

    challenges_init();
    register_content_pack_arrays();

    banner();
    printf("Welcome, %s. Type number and press Enter.\n", G_PROFILE.name);

    for (;;) {
        show_profile();
        printf("\nMenu:\n");
        printf(" 1) Overworld map\n");
        printf(" 2) Enter Quest -> Coding Challenge\n");
        printf(" 3) Run tests -> Reward/XP\n");
        printf(" 4) Skill tree -> Unlock content\n");
        printf(" 5) Save/Cloud sync\n");
        printf(" 0) Exit\n> ");

        int choice = -1; if (scanf("%d", &choice) != 1) break; while (getchar()!='\n' && !feof(stdin));
        switch (choice) {
            case 1: overworld(); break;
            case 2: enter_quest(); break;
            case 3: {
                const Challenge *c = challenges_get(0);
                if (!c) { printf("No challenges registered.\n"); break; }
                GradeResult r = challenges_grade(c, c->visibility);
                printf("\nResult: %d/%d passed\n", r.passed, r.total);
                if (r.passed == r.total) {
                    printf("Reward: +%d XP\n", c->xp_reward);
                    G_PROFILE.xp += c->xp_reward;
                    G_PROFILE.level = xp_to_level(G_PROFILE.xp);
                    G_PROFILE.challenges_solved += 1;
                    Event ev1 = { .type = EV_XP_GAIN, .i1 = c->xp_reward, .s1 = c->slug };
                    eventbus_publish(&G_BUS, &ev1);
                }
                break;
            }
            case 4: skill_tree(); break;
            case 5: save_now(); break;
            case 0: save_now(); printf("Bye.\n"); return 0;
            default: printf("Unknown.\n"); break;
        }
    }
    return 0;
}
