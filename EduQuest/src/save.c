// =============================================
// file: src/save.c
// =============================================
#include "save.h"

static void ensure_dir(const char *path){
#ifdef _WIN32
    _mkdir(path);
#else
    mkdir(path,0755);
#endif
}

char *get_user_dir(char *buf,size_t n){
#ifdef _WIN32
    const char *base=getenv("LOCALAPPDATA"); if(!base) base=getenv("APPDATA"); if(!base) base=".";
#else
    const char *base=getenv("HOME"); if(!base) base=".";
#endif
    snprintf(buf,n,"%s",base); return buf;
}

char *get_save_dir(char *buf,size_t n){ char base[512]; get_user_dir(base,sizeof base);
#ifdef _WIN32
    snprintf(buf,n,"%s%c%s",base,PATH_SEP,EDUQ_APPNAME);
#else
    snprintf(buf,n,"%s%c.%s",base,PATH_SEP,EDUQ_APPNAME);
#endif
    ensure_dir(buf); return buf;
}

char *get_save_path(char *buf,size_t n){ char d[512]; get_save_dir(d,sizeof d); snprintf(buf,n,"%s%cprofile.txt",d,PATH_SEP); return buf; }
char *get_analytics_path(char *buf,size_t n){ char d[512]; get_save_dir(d,sizeof d); snprintf(buf,n,"%s%canalytics.csv",d,PATH_SEP); return buf; }

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
    return true;
}

bool load_profile(Profile *p) {
    memset(p, 0, sizeof *p);
    strncpy(p->name, "Adventurer", sizeof p->name - 1);
    p->xp = 0; p->level = 1; p->challenges_solved = 0;

    char path[512];
    get_save_path(path, sizeof path);
    FILE *f = fopen(path, "r");
    if (!f) return true;

    char line[256];
    while (fgets(line, sizeof line, f)) {
        clamp_line(line);
        char k[64], v[128];
        if (sscanf(line, "%63[^=]=%127s", k, v) == 2) {
            if (strcmp(k, "name") == 0) strncpy(p->name, v, sizeof p->name - 1);
            else if (strcmp(k, "xp") == 0) p->xp = atoi(v);
            else if (strcmp(k, "level") == 0) p->level = atoi(v);
            else if (strcmp(k, "solved") == 0) p->challenges_solved = atoi(v);
        }
    }
    fclose(f);
    p->level = xp_to_level(p->xp);
    return true;
}
