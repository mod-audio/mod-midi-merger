#ifndef MIDI_MERGER_H
#define MIDI_MERGER_H

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <jack/jack.h>
#include <jack/midiport.h>
#include <jack/ringbuffer.h>
#include <pthread.h>
#include <stdbool.h>

enum Ports {
    PORT_IN,
    PORT_OUT,
    PORT_ARRAY_SIZE // this is not used as a port index
};

static const size_t queue_size = 64*sizeof(jack_port_id_t);

typedef struct MIDI_MERGER_T {
  jack_client_t *client;
  jack_port_t *ports[PORT_ARRAY_SIZE];
  jack_ringbuffer_t *ports_to_connect;

  bool do_exit;
  pthread_t connection_supervisor;
} midi_merger_t;

/**
 * For use as a Jack-internal client, `jack_initialize()` and
 * `jack_finish()` have to be exported in the shared library.
 */
__attribute__ ((visibility("default")))
int jack_initialize(jack_client_t* client, const char* load_init);

__attribute__ ((visibility("default")))
void jack_finish(void* arg);

#endif
