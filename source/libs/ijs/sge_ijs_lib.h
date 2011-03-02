/* Portions of this code are Copyright 2011 Univa Inc. */

int continue_handler (COMM_HANDLE *comm_handle, char *hostname); 
int suspend_handler (COMM_HANDLE *comm_handle, char *hostname, int b_is_rsh, unsigned int pid, dstring *dbuf);
