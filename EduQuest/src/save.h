// =============================================
// file: src/save.h
// =============================================
#ifndef EDUQ_SAVE_H
#define EDUQ_SAVE_H
#include "common.h"
#include "profile.h"

char *get_user_dir(char *buf, size_t bufsz);
char *get_save_dir(char *buf, size_t bufsz);
char *get_save_path(char *buf, size_t bufsz);
char *get_analytics_path(char *buf, size_t bufsz);

bool save_profile(const Profile *p);
bool load_profile(Profile *p);

#endif // EDUQ_SAVE_H

