/* Portions of this code are Copyright 2011 Univa Inc. */
#include <termio.h>
#include "sgermon.h"

int continue_handler (COMM_HANDLE *comm_handle, char *hostname) {
  DENTER(TOP_LAYER, "ijs_suspend: continue_handler");
  DEXIT;
  return 0;
}

int suspend_handler (COMM_HANDLE *comm_handle, char *hostname, int b_is_rsh, unsigned int pid, dstring *dbuf) {
  DENTER(TOP_LAYER, "ijs_suspend: suspend_handler");
  DEXIT;
  return 1;
}
