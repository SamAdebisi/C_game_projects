#include "common.h"
#include "save.h"
#include "analytics.h"

static int file_exists(const char *p){ FILE*f=fopen(p,"r"); if(!f) return 0; fclose(f); return 1; }

static void now_iso(char *buf,size_t n){
    time_t t=time(NULL);
    struct tm tmv;
#ifdef _WIN32
    localtime_s(&tmv,&t);
#else
    localtime_r(&t,&tmv);
#endif
    strftime(buf,n,"%Y-%m-%dT%H:%M:%S",&tmv);
}

void analytics_log_header_if_needed(void){
    char path[512]; get_analytics_path(path,sizeof path);
    if (file_exists(path)) return;
    FILE*f=fopen(path,"w"); if(!f) return;
    fprintf(f,"timestamp,kind,detail,value\n");
    fclose(f);
}

void analytics_log_event(const char *kind,const char *detail,int v){
    char path[512]; get_analytics_path(path,sizeof path);
    FILE*f=fopen(path,"a"); if(!f) return;
    char ts[32]; now_iso(ts,sizeof ts);
    fprintf(f,"%s,%s,%s,%d\n",ts,kind?kind:"",detail?detail:"",v);
    fclose(f);
} 