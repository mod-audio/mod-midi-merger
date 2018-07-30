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



int jack_initialize(jack_client_t* client, const char* load_init)
{
  /* allocate monitor client */
  midi_merger_t *const mm = malloc(sizeof(midi_merger_t));

  

}


void jack_finish(void* arg)
{
  midi_merger_t *const mm = (midi_merger_t *const) arg;

  /* jack_deactivate(mon->client); */
  
  /* g_active = false; */
  
  /* for (int i=0; i<PORT_COUNT; ++i) */
  /*   jack_port_unregister(mon->client, mon->ports[i]); */
  
  free(mm);
}
