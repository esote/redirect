/*
 * redirect is a web server for subdomain redirects.
 * Copyright (C) 2019 Esote
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include <netinet/in.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "redirect.h"

static int sfd, afd;
static struct route *routes;

static void
sigint_handler(int sig, siginfo_t *info, void *ucontext)
{
	(void)sig;
	(void)info;
	(void)ucontext;

	/* Try to be polite while we're on our way out. */
	config_free(routes);
	(void)close(sfd);
	(void)close(afd);
	_exit(0);
}

int
main(int argc, char *argv[])
{
	struct sigaction act;
	struct sockaddr_in addr;
	socklen_t addrlen;
	struct timeval tv;
	int opt;

	addrlen = sizeof(addr);
	opt = 1;

	if (argc < 2) {
		fprintf(stderr, "usage: %s file\n", argv[0]);
		return 1;
	}

	if ((routes = config(argv[1])) == NULL) {
		err(1, "config");
	}

	if (geteuid() == 0 && chroot(".") == -1) {
		err(1, "chroot");
	}

#ifdef __OpenBSD__
	if (pledge("stdio inet", "") == -1) {
		err(1, "pledge");
	}
#endif

	if ((sfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		err(1, "socket");
	}

	if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
		err(1, "setsockopt SO_REUSEADDR");
	}

	(void)memset(&act, 0, sizeof(act));

	if (sigemptyset(&act.sa_mask) == -1) {
		err(1, "sigemptyset");
	}

	act.sa_sigaction = sigint_handler;

	if (sigaction(SIGINT, &act, NULL) == -1) {
		err(1, "sigaction");
	}

	(void)memset(&addr, 0, sizeof(addr));

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(PORT);

	if (bind(sfd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
		err(1, "bind");
	}

	if (listen(sfd, QUEUE_LEN) == -1) {
		err(1, "listen");
	}

	tv.tv_sec = T_SEC;
	tv.tv_usec = 0;

	while (1) {
		if ((afd = accept(sfd, (struct sockaddr *)&addr, &addrlen)) == -1) {
			warn("accept");
			continue;
		}

		if (setsockopt(afd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == -1) {
			warn("setsockopt SO_RCVTIMEO");
			goto done;
		}

		if (setsockopt(afd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) == -1) {
			warn("setsockopt SO_SNDTIMEO");
			goto done;
		}

		respond(afd, routes);

		if (shutdown(afd, SHUT_RDWR) == -1) {
			warn("shutdown rdwr");
		}

done:
		if (close(afd) == -1) {
			warn("close afd");
		}
	}
}
