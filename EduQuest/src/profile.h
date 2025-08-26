#ifndef EDUQ_PROFILE_H
#define EDUQ_PROFILE_H
typedef struct { char name[64]; int xp; int level; int challenges_solved; } Profile;
static inline int xp_to_level(int xp){ return xp/100 + 1; }
#endif 