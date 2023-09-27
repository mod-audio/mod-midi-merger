#include "midi-broadcaster.h"

#include <unistd.h>

/* port flags to connect to */
static const int target_port_flags = JackPortIsTerminal|JackPortIsPhysical|JackPortIsInput;

/**
 * Add a port_id to the waiting queue.
 * It is a producer in the realtime context.
 */
size_t push_back(jack_ringbuffer_t *queue, jack_port_id_t port_id) {
  size_t written = 0;

  if (queue == NULL) {
    fprintf(stderr, "Could not schedule port connection.\n");
  } else {

    // Check if there is space to write
    size_t write_space = jack_ringbuffer_write_space(queue);
    if (write_space >= sizeof(jack_port_id_t)) {
      written = jack_ringbuffer_write(queue,
                                      (const char*)&port_id,
                                      sizeof(jack_port_id_t));
      if (written < sizeof(jack_port_id_t)) {
        fprintf(stderr, "Could not schedule port connection.\n");
      }
    }
  }
  return written;
}


/**
 * Return the next port_id or NULL if empty.
 */
jack_port_id_t next(jack_ringbuffer_t *queue) {
  size_t read = 0;
  jack_port_id_t id = 0;

  if (queue == NULL) {
    fprintf(stderr, "Queue memory problem.\n");
  } else {
    char buffer[sizeof(jack_port_id_t)];
    read = jack_ringbuffer_read(queue, buffer, sizeof(jack_port_id_t));
    if (read == sizeof(jack_port_id_t)) {
      id = (jack_port_id_t) *buffer;
    }
  }
  return id;
}

/**
 * Connect any outstanding Jack ports.
 * It is a consumer in the non-realtime context.
 */
void handle_scheduled_connections(midi_broadcaster_t *const mm) {
  // Check if there are connections scheduled.
  jack_port_id_t source;

  while ((source = next(mm->ports_to_connect)) != 0) {
    int result;
    result = jack_connect(mm->client,
                          jack_port_name(mm->ports[PORT_OUT]) ,
                          jack_port_name(jack_port_by_id(mm->client, source)));
    switch(result) {
    case 0:
      // Fine.
      break;
    case EEXIST:
      fprintf(stderr, "Connection exists.\n");
      break;
    default:
      fprintf(stderr, "Could not connect port.\n");
      break;
    }
  }
}


static int process_callback(jack_nframes_t nframes, void *arg)
{
  // nothing here, `jack_port_tie` takes care of buffer zero-copy
  return 0;
}


static void port_registration_callback(jack_port_id_t port_id, int is_registered, void *arg)
{
  midi_broadcaster_t *const mm = (midi_broadcaster_t *const) arg;

  // If there is a new port of type MIDI input we connect it to our
  // output port.
  if (is_registered) {
    jack_port_t *source = jack_port_by_id(mm->client, port_id);
    // Check if MIDI input
    if ((jack_port_flags(source) & target_port_flags) == target_port_flags) {
      const char *const ptype = jack_port_type(source);
      if (ptype && strcmp(ptype, JACK_DEFAULT_MIDI_TYPE) == 0) {

        // We can't call jack_connect here in the callback,
        // Schedule the connection for later.
        push_back(mm->ports_to_connect, port_id);
        sem_post(&mm->sem);
      }
    }
  }
  return;
}


/**
 * `Supervise` handles the non-realtime port connections.
 */
void *supervise(void *arg) {
  midi_broadcaster_t *const mm = (midi_broadcaster_t *const) arg;

  while (mm->do_exit == false) {
    handle_scheduled_connections(mm);
    sem_wait(&mm->sem);
  }
  return NULL;
}

int jack_initialize(jack_client_t* client, const char* load_init)
{
  midi_broadcaster_t *const mm = malloc(sizeof(midi_broadcaster_t));
  if (!mm) {
    fprintf(stderr, "Out of memory\n");
    return EXIT_FAILURE;
  }

  mm->client = client;

  // Register ports.
  mm->ports[PORT_IN] = jack_port_register(client, "in",
                                          JACK_DEFAULT_MIDI_TYPE,
                                          JackPortIsInput, 0);
  mm->ports[PORT_OUT] = jack_port_register(client, "out",
                                           JACK_DEFAULT_MIDI_TYPE,
                                           JackPortIsOutput, 0);
  for (int i = 0; i < PORT_ARRAY_SIZE; ++i) {
    if (!mm->ports[i]) {
      fprintf(stderr, "Can't register jack port\n");
      free(mm);
      return EXIT_FAILURE;
    }
  }

  jack_port_tie(mm->ports[PORT_IN], mm->ports[PORT_OUT]);

  // Set port aliases
  jack_port_set_alias(mm->ports[PORT_IN], "MIDI in");
  jack_port_set_alias(mm->ports[PORT_OUT], "MIDI out");

  // Create the ringbuffer (single-producer/single-consumer) for
  // scheduled port connections. It contains elements of type
  // `jack_port_id_t`.
  mm->ports_to_connect = jack_ringbuffer_create(queue_size);

  // Set callbacks
  jack_set_process_callback(client, process_callback, mm);
  jack_set_port_registration_callback(client, port_registration_callback, mm);

  // Init the connection supervisor worker thread
  sem_init(&mm->sem, 0, 0);
  mm->do_exit = false;
  int rc = pthread_create(&(mm->connection_supervisor), NULL, &supervise, mm);
  if (rc != 0) {
    fprintf(stderr, "Can't create worker thread\n");
    return EXIT_FAILURE;
  }

  /* Activate the jack client */
  if (jack_activate(client) != 0) {
    fprintf(stderr, "can't activate jack client\n");
    free(mm);
    return EXIT_FAILURE;
  }

  const char **const ports = jack_get_ports(client, "", JACK_DEFAULT_MIDI_TYPE, target_port_flags);
  if (ports != NULL)
  {
    char  aliases[2][320];
    char* aliasesptr[2] = { aliases[0], aliases[1] };
    const char* const ourportname = jack_port_name(mm->ports[PORT_OUT]);

    for (int i=0; ports[i] != NULL; ++i) {
      if (strncmp(ports[i], "system:midi_playback_", 21) == 0) {
        if (jack_port_get_aliases(jack_port_by_name(client, ports[i]), aliasesptr) > 0) {
          if (strncmp(aliases[0], "alsa_pcm:Midi-Through/", 22) == 0) {
            continue;
          }
        }
      }
      jack_connect(client, ourportname, ports[i]);
    }

    jack_free(ports);
  }

  return 0;
}


void jack_finish(void* arg)
{
  midi_broadcaster_t *const mm = (midi_broadcaster_t *const) arg;

  jack_deactivate(mm->client);

  for (int i = 0; i < PORT_ARRAY_SIZE; ++i) {
    jack_port_unregister(mm->client, mm->ports[i]);
  }

  mm->do_exit = true;
  sem_post(&mm->sem);
  pthread_join(mm->connection_supervisor, NULL);
  sem_destroy(&mm->sem);
  jack_ringbuffer_free(mm->ports_to_connect);

  free(mm);
}
