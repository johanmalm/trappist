/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef TRAPPIST_IPC_H
#define TRAPPIST_IPC_H
#include <stdbool.h>

/*
 * Try to send a "show" message to an already-running trappist instance.
 * Returns true if a running instance was found and the message was sent,
 * false if no running instance exists (caller should start one).
 */
bool ipc_client_send_show(void);

/*
 * Create the Unix-socket IPC server and write the lockfile.
 * Returns the listening socket fd (>= 0) on success, -1 on failure.
 */
int ipc_server_init(void);

/* Remove the socket file and lockfile created by ipc_server_init(). */
void ipc_cleanup(void);

#endif /* TRAPPIST_IPC_H */
