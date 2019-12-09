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
#include <sys/stat.h>

#include <ctype.h>
#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "redirect.h"

struct route *
config(char *name)
{
	char *f;
	struct route *r;
	struct stat st;
	size_t f_l;
	size_t n;
	int fd;
	char *t, *e1, *e2;

	r = NULL;
	n = 0;

	if ((fd = open(name, O_RDONLY)) == -1) {
		return NULL;
	}

	if (fstat(fd, &st) == -1) {
		return NULL;
	}

	if (st.st_size == 0) {
		errx(1, "empty config");
	}

	f_l = (size_t)st.st_size + 1;

	if ((f = mmap(NULL, f_l, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0)) == MAP_FAILED) {
		return NULL;
	}

	f[f_l - 1] = '\0';

	if (close(fd) == -1) {
		return NULL;
	}

	/* Tokenize line-separated routes between space-separated hosts. */
	for (t = strtok_r(f, NL, &e1); t != NULL; t = strtok_r(NULL, NL, &e1)) {
		if ((r = realloc(r, (n + 1) * sizeof(struct route))) == NULL) {
			return NULL;
		}

		if ((t = strtok_r(t, SP, &e2)) == NULL) {
			errx(1, "malformed config");
		}

		if ((r[n].from = strdup(t)) == NULL) {
			return NULL;
		}

		if ((t = strtok_r(NULL, SP, &e2)) == NULL) {
			errx(1, "malformed config");
		}

		if ((r[n].to = strdup(t)) == NULL) {
			return NULL;
		}

		n++;
	}

	if (n == 0) {
		errx(1, "no routes parsed");
	}

	if ((r = realloc(r, (n + 1) * sizeof(struct route))) == NULL) {
		return NULL;
	}

	(void)memset(&r[n], 0, sizeof(struct route));

	if (munmap(f, f_l) == -1) {
		return NULL;
	}

	return r;
}

void
config_free(struct route *r)
{
	struct route *rr = r;
	while (rr->from != NULL)
	{
		free(rr->from);
		free(rr->to);
		rr++;
	}
	free(r);
}
