#include <stdlib.h>
#include <stdio.h>
#include <jack/jack.h>


enum Ports {
    PORT_IN,
    PORT_OUT,
    PORT_COUNT // this is used as an array index
};

typedef struct MIDI_MERGER_T {
  jack_client_t *client;
  jack_port_t *ports[PORT_COUNT];
} midi_merger_t;

/**
 * For use as a Jack-internal client, `jack_initialize()` and
 * `jack_finish()` have to be exported in the shared library.
 */
__attribute__ ((visibility("default")))
int jack_initialize(jack_client_t* client, const char* load_init);

__attribute__ ((visibility("default")))
void jack_finish(void* arg);


static int process_callback(jack_nframes_t nframes, void *arg)
{
  midi_merger_t *const mm = (midi_merger_t *const) arg;

  // TODO: MIDI events through

  return 0;
}


int jack_initialize(jack_client_t* client, const char* load_init)
{
  midi_merger_t *const mm = malloc(sizeof(midi_merger_t));
  if (!mm) {
    fprintf(stderr, "Out of memory\n");
    return EXIT_FAILURE;
  }

  mm->client = client;

  // Register ports. We fake a physical output port.
  mm->ports[PORT_IN] = jack_port_register(client, "in",
					  JACK_DEFAULT_MIDI_TYPE,
					  JackPortIsInput, 0);
  mm->ports[PORT_OUT] = jack_port_register(client, "out",
					   JACK_DEFAULT_MIDI_TYPE,
					   JackPortIsOutput | JackPortIsPhysical, 0);
  for (int i = 0; i < PORT_COUNT; ++i) {
    if (!mm->ports[i]) {
      fprintf(stderr, "Can't register jack port\n");
      free(mm);
      return EXIT_FAILURE;
    }
  }

  // Set callback
  jack_set_process_callback(client, process_callback, mm);

  /* Activate the jack client */
  if (jack_activate(client) != 0) {
    fprintf(stderr, "can't activate jack client\n");
    free(mm);
    return EXIT_FAILURE;
  }

  return 0;
}


void jack_finish(void* arg)
{
  midi_merger_t *const mm = (midi_merger_t *const) arg;

  jack_deactivate(mm->client);
  
  for (int i = 0; i < PORT_COUNT; ++i) {
    jack_port_unregister(mm->client, mm->ports[i]);
  }
  
  free(mm);
}
