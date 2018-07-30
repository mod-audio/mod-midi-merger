#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <jack/jack.h>
#include <jack/midiport.h>


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

  // Get and clean the output buffer once per cycle.
  void *output_port_buffer = jack_port_get_buffer(mm->ports[PORT_OUT], nframes);
  jack_midi_clear_buffer(output_port_buffer);

  // Copy events from the input to the output.
  void *input_port_buffer = jack_port_get_buffer(mm->ports[PORT_IN], nframes);
  jack_nframes_t event_count = jack_midi_get_event_count(input_port_buffer); 
  if (event_count > 0) {
    
    jack_midi_event_t in_event;
    for (jack_nframes_t i = 0; i < event_count; ++i) {
      jack_midi_event_get(&in_event, input_port_buffer, i);
      
      int result;
      result = jack_midi_event_write(output_port_buffer,
				     in_event.time, in_event.buffer, in_event.size);
      switch(result) {
      case 0:
  	// Fine.
  	break;
      case ENOBUFS:
  	fprintf(stderr, "Not enough space for MIDI event.\n");
  	// Fall through
      default:
  	fprintf(stderr, "Could not write MIDI event.\n");
  	break;
      }      
    }
  }

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


/**
 * For testing purposes.
 */
int main() {
  int result = EXIT_FAILURE;
  jack_options_t options = JackNoStartServer;
  jack_status_t status;
  jack_client_t *client = jack_client_open("midi-merger", options, &status);
  if (client == NULL) {
    fprintf(stderr, "Opening client failed. Status is %d.\n", status);
    if (status & JackServerFailed) {
      fprintf(stderr, "Unable to connect to Jack server.\n");
    }
    return EXIT_FAILURE;
  }
  
  result = jack_initialize(client, "");

  while (1) {}
  
  jack_finish(client);
  
  return result;
}
