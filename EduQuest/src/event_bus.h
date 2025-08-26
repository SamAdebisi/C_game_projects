#ifndef EDUQ_EVENT_BUS_H
#define EDUQ_EVENT_BUS_H
#include <stddef.h>
#include <stdbool.h>

typedef enum { EV_NONE=0, EV_XP_GAIN, EV_CHALLENGE_PASSED, EV_SAVED } EventType;

typedef struct { EventType type; int i1; const char *s1; } Event;

typedef void (*EventHandler)(const Event *ev, void *user);

#define MAX_SUBS 64
typedef struct { EventHandler handlers[MAX_SUBS]; void* user[MAX_SUBS]; size_t count; } EventBus;

void eventbus_init(EventBus *bus);
bool eventbus_subscribe(EventBus *bus, EventHandler h, void *user);
void eventbus_publish(EventBus *bus, const Event *ev);
#endif