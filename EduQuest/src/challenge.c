// =============================================
// file: src/challenge.c
// =============================================
#include "challenge.h"

#define MAX_CHALLENGES 64
static Challenge g_chals[MAX_CHALLENGES];
static int g_chal_count=0;

void challenges_init(void){ g_chal_count=0; }
int  challenges_register(const Challenge *c){ if(g_chal_count>=MAX_CHALLENGES) return -1; g_chals[g_chal_count]=*c; g_chals[g_chal_count].id=g_chal_count; return g_chal_count++; }
int  challenges_count(void){ return g_chal_count; }
const Challenge* challenges_get(int idx){ if(idx<0||idx>=g_chal_count) return NULL; return &g_chals[idx]; }

GradeResult challenges_grade(const Challenge *c,int visibility){ GradeResult r={0,0}; if(!c) return r; if(c->sig==SIG_SUM_ARRAY){ fn_sum_array fn=(fn_sum_array)c->solution_fn; for(size_t i=0;i<c->case_count;i++){ const SumArrayCase *tc=&c->cases[i]; int got=fn(tc->input,tc->n); r.total++; if(got==tc->expected) r.passed++; else if(visibility>0){ printf("  • Case %zu failed: expected %d got %d
", i+1, tc->expected, got); if(visibility>1 && tc->hint) printf("    hint: %s
", tc->hint); } } } return r; }

