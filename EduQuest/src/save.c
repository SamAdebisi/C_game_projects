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