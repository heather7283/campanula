#ifndef SRC_TUI_EVENTS_H
#define SRC_TUI_EVENTS_H

#include <stdint.h>

#include "signals.h"

void tui_handle_resize(int width, int height);
void tui_handle_key(uint32_t key);
void tui_handle_player_events(uint64_t event, const struct signal_data *data, void *userdata);
void tui_handle_network_events(uint64_t event, const struct signal_data *data, void *userdata);

#endif /* #ifndef SRC_TUI_EVENTS_H */

