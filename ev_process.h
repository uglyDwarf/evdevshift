#ifndef EV_PROCESS__H
#define EV_PROCESS__H

#include <stdbool.h>
#include <linux/input.h>

extern int send_event(struct input_event *ev);

bool process_event(struct input_event *event);

#endif