#ifndef EDUQ_ANALYTICS_H
#define EDUQ_ANALYTICS_H
void analytics_log_header_if_needed(void);
void analytics_log_event(const char *kind, const char *detail, int v);
#endif
