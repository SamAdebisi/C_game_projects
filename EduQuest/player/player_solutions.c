// =============================================
// file: player/player_solutions.c
// =============================================
#include <stddef.h>
#include <limits.h>

int sum_array(const int *a, size_t n){ long long acc=0; for(size_t i=0;i<n;i++) if(a) acc+=a[i]; if(acc>INT_MAX) acc=INT_MAX; if(acc<INT_MIN) acc=INT_MIN; return (int)acc; }
