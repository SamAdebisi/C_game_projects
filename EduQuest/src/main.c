#include "common.h"
#include "profile.h"
#include "event_bus.h"
#include "save.h"
#include "analytics.h"
#include "challenge.h"
#include "content_arrays.h"

static EventBus G_BUS;
static Profile  G_PROFILE;

static void on_event(const Event *ev, void *u){
    (void)u;
    if (ev->type == EV_XP_GAIN) analytics_log_event("xp_gain", ev->s1, ev->i1);
    else if (ev->type == EV_CHALLENGE_PASSED) analytics_log_event("challenge_pass", ev->s1, ev->i1);
    else if (ev->type == EV_SAVED) analytics_log_event("saved", "profile", 1);
}

static void banner(void){ printf("\n== %s v%s ==\n", EDUQ_APPNAME, EDUQ_VERSION); }

static void show_profile(void){
    printf("\nProfile: %s | XP: %d | Level: %d | Solved: %d\n",
           G_PROFILE.name, G_PROFILE.xp, G_PROFILE.level, G_PROFILE.challenges_solved);
}

static void ensure_profile_named(void){
    if (strcmp(G_PROFILE.name, "Adventurer") == 0) {
        printf("Enter profile name: ");
        char buf[64];
        if (fgets(buf, sizeof buf, stdin)) clamp_line(buf);
        if (buf[0]) strncpy(G_PROFILE.name, buf, sizeof G_PROFILE.name - 1);
    }
}

static void overworld(void){
    printf("\n[Overworld] Zones: Arrays (1) | Recursion (locked) | OOP (locked)\n");
}

static void skill_tree(void){
    printf("\n[Skill Tree]\n  Fundamentals %s\n  Arrays %s\n  Recursion %s\n",
           (G_PROFILE.level>=1? "[x]": ""),
           (G_PROFILE.challenges_solved>=1? "[x]": ""),
           (G_PROFILE.level>=3? "Unlocked": "Locked"));
}

static void do_list_challenges(void){
    int n = challenges_count();
    printf("\nQuests available (%d):\n", n);
    for (int i = 0; i < n; ++i) {
        const Challenge *c = challenges_get(i);
        printf("  [%d] %s - %s\n", i, c->name, c->slug);
    }
}

static const Challenge* select_challenge(void){
    do_list_challenges();
    printf("Select quest id: ");
    char b[32]; if (!fgets(b, sizeof b, stdin)) return NULL;
    int id = (int)strtol(b, NULL, 10);
    return challenges_get(id);
}

static void enter_quest(void){
    const Challenge *c = select_challenge();
    if (!c) { printf("Invalid selection.\n"); return; }
    printf("\nQuest: %s\n%s\n", c->name, c->description);
    printf("Run tests now? [y/N]: ");
    int ch = getchar(); while (getchar()!='\n' && !feof(stdin));
    if (ch=='y' || ch=='Y') {
        GradeResult r = challenges_grade(c, c->visibility);
        printf("\nResult: %d/%d passed\n", r.passed, r.total);
        if (r.passed == r.total) {
            printf("Reward: +%d XP\n", c->xp_reward);
            int lvl_before = G_PROFILE.level;
            G_PROFILE.xp += c->xp_reward;
            G_PROFILE.level = xp_to_level(G_PROFILE.xp);
            G_PROFILE.challenges_solved += 1;
            Event ev1 = { .type = EV_XP_GAIN, .i1 = c->xp_reward, .s1 = c->slug };
            Event ev2 = { .type = EV_CHALLENGE_PASSED, .i1 = 1, .s1 = c->slug };
            eventbus_publish(&G_BUS, &ev1);
            eventbus_publish(&G_BUS, &ev2);
            if (G_PROFILE.level > lvl_before) printf("Level up -> %d\n", G_PROFILE.level);
        } else {
            printf("Edit code in player/player_solutions.c and rebuild.\n");
        }
    } else {
        printf("Use 'Enter Quest' again when ready.\n");
    }
}

static void run_default_tests(void){
    const Challenge *c = challenges_get(0);
    if (!c) { printf("No challenges registered.\n"); return; }
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
}

static void save_now(void){
    if (save_profile(&G_PROFILE)) {
        printf("Saved.\n");
        Event ev = { .type = EV_SAVED };
        eventbus_publish(&G_BUS, &ev);
    } else {
        printf("Save failed.\n");
    }
}

int main(void){
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
        printf("\nMenu:\n"
               " 1) Overworld map\n"
               " 2) Enter Quest -> Coding Challenge\n"
               " 3) Run tests -> Reward/XP\n"
               " 4) Skill tree -> Unlock content\n"
               " 5) Save/Cloud sync\n"
               " 0) Exit\n> ");
        char b[32]; if (!fgets(b, sizeof b, stdin)) break;
        int choice = (int)strtol(b, NULL, 10);
        switch (choice) {
            case 1: overworld(); break;
            case 2: enter_quest(); break;
            case 3: run_default_tests(); break;
            case 4: skill_tree(); break;
            case 5: save_now(); break;
            case 0: save_now(); printf("Bye.\n"); return 0;
            default: printf("Unknown.\n"); break;
        }
    }
    return 0;
}