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

#include <sys/socket.h>

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "redirect.h"

void
respond(int fd, struct route *r)
{
	static char buf[BUF_LEN];
	static char resp[BUF_LEN];
	ssize_t n;
	char *line, *word, *lastLine, *lastWord;
	char *path;

	if ((n = read(fd, buf, BUF_LEN-1)) == -1) {
		warn("read");
		return;
	}

	buf[n] = '\0';

	if (shutdown(fd, SHUT_RD) == -1) {
		warn("shutdown rd");
		return;
	}

	if ((line = strtok_r(buf, NL, &lastLine)) == NULL) {
		/* Malformed HTTP request line. */
		return;
	}

	if ((word = strtok_r(line, SP, &lastWord)) == NULL) {
		/* Malformed HTTP request line. */
		return;
	}

	if (strcmp(word, "GET") != 0) {
		/* Invalid HTTP verb. */
		return;
	}

	if ((path = strtok_r(NULL, SP NL, &lastWord)) == NULL) {
		/* Malformed HTTP request path. */
		return;
	}

	while (1) {
		if ((line = strtok_r(NULL, NL, &lastLine)) == NULL) {
			/* Malformed HTTP request headers. */
			return;
		}

		if ((word = strtok_r(line, SP, &lastWord)) == NULL) {
			continue;
		}

		/* Headers are technically case-insensitive, but too bad. */
		if (strcmp(word, "Host:") != 0) {
			continue;
		}

		if ((word = strtok_r(NULL, SP, &lastWord)) != NULL) {
			break;
		}
	}

	for (; r->from != NULL && strcmp(r->from, word) != 0; r++);

	if (r->from == NULL) {
		/* No route from host. */
		return;
	}

	n = snprintf(resp, BUF_LEN, "HTTP/1.1 " HTTP_CODE " " HTTP_TEXT "\n"
		"Content-Length: %zu\n"
		"Content-Type: text/html; charset=utf-8\n"
		"Location: %s%s\n\n"
		"<a href=\"%s%s\">" HTTP_TEXT "</a>.\n\n",
		BODY_LEN + strlen(r->to) + strlen(path), r->to, path, r->to,
		path);

	if (n < 0) {
		warn("snprintf");
		return;
	}

	if (write(fd, resp, (size_t)n) == -1) {
		warn("write");
	}
}
