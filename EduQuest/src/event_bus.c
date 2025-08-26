#include "event_bus.h"

void eventbus_init(EventBus *bus) { bus->count = 0; }

bool eventbus_subscribe(EventBus *bus, EventHandler h, void *user) {
    if (bus->count >= MAX_SUBS) return false;
    bus->handlers[bus->count] = h;
    bus->user[bus->count] = user;
    bus->count++;
    return true;
}

void eventbus_publish(EventBus *bus, const Event *ev) {
    for (size_t i = 0; i < bus->count; ++i) bus->handlers[i](ev, bus->user[i]);
}