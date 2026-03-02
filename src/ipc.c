// SPDX-License-Identifier: GPL-2.0-only
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <sway-client-helpers/log.h>
#include "ipc.h"

static char socket_path[256];
static char lock_path[256];
static int server_fd = -1;

static void
get_paths(void)
{
	const char *runtime_dir = getenv("XDG_RUNTIME_DIR");
	if (!runtime_dir) {
		runtime_dir = "/tmp";
	}
	snprintf(socket_path, sizeof(socket_path), "%s/trappist.sock",
		runtime_dir);
	snprintf(lock_path, sizeof(lock_path), "%s/trappist.lock",
		runtime_dir);
}

bool
ipc_client_send_show(void)
{
	get_paths();

	FILE *f = fopen(lock_path, "r");
	if (!f) {
		return false;
	}
	pid_t pid = 0;
	int ret = fscanf(f, "%d", &pid);
	fclose(f);
	if (ret != 1 || pid <= 0) {
		return false;
	}

	/* Check whether the process is still alive */
	if (kill(pid, 0) != 0) {
		/* Stale lockfile — clean up so the caller can start fresh */
		unlink(lock_path);
		unlink(socket_path);
		return false;
	}

	int fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd < 0) {
		return false;
	}

	struct sockaddr_un addr = { 0 };
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

	if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
		close(fd);
		return false;
	}

	if (write(fd, "show", 4) != 4) {
		close(fd);
		return false;
	}
	close(fd);
	return true;
}

int
ipc_server_init(void)
{
	get_paths();

	/* Write PID to lockfile */
	FILE *f = fopen(lock_path, "w");
	if (!f) {
		LOG(LOG_ERROR, "failed to create lockfile '%s'", lock_path);
		return -1;
	}
	fprintf(f, "%d\n", getpid());
	fclose(f);

	server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (server_fd < 0) {
		LOG(LOG_ERROR, "failed to create IPC socket");
		unlink(lock_path);
		return -1;
	}

	int flags = fcntl(server_fd, F_GETFL, 0);
	fcntl(server_fd, F_SETFL, flags | O_NONBLOCK);

	/* Remove any stale socket file */
	unlink(socket_path);

	struct sockaddr_un addr = { 0 };
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

	if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
		LOG(LOG_ERROR, "failed to bind IPC socket");
		close(server_fd);
		unlink(lock_path);
		server_fd = -1;
		return -1;
	}

	if (listen(server_fd, 5) != 0) {
		LOG(LOG_ERROR, "failed to listen on IPC socket");
		close(server_fd);
		unlink(socket_path);
		unlink(lock_path);
		server_fd = -1;
		return -1;
	}

	return server_fd;
}

void
ipc_cleanup(void)
{
	if (server_fd >= 0) {
		close(server_fd);
		server_fd = -1;
	}
	unlink(socket_path);
	unlink(lock_path);
}
