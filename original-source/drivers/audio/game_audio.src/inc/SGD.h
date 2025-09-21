
#ifndef __SGD_H__
#define __SGD_H__

#include "wrapper.h"

/* SGD functions */
status_t init_SGD(sound_card_t *card);
void uninit_SGD(void);
uint32 create_noise_buffer(void);
status_t fill_table_from_queue(int16 streamID);
bool clear_current_entry(flow_dir flow_type, audiobuffer_node *cleared_entry_info);
status_t increment_readpoint(flow_dir flow_type);
SGD *get_SGD(int16 streamID);
int16 *get_buffer_table(int16 streamID);
void print_SGD(flow_dir flow_type);

#endif
