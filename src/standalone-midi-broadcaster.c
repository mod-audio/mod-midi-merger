#include "midi-broadcaster.h"
#include <unistd.h>

int main() {
  int result = EXIT_FAILURE;
  jack_options_t options = JackNoStartServer;
  jack_status_t status;
  jack_client_t *client = jack_client_open("mod-midi-broadcaster", options, &status);
  if (client == NULL) {
    fprintf(stderr, "Opening client failed. Status is %d.\n", status);
    if (status & JackServerFailed) {
      fprintf(stderr, "Unable to connect to Jack server.\n");
    }
    return EXIT_FAILURE;
  }

  result = jack_initialize(client, "");

  while (1) {
    sleep(60);
  }

  jack_finish(client);

  return result;
}
