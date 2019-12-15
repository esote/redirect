#include <err.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <fuzz.h> /* see https://github.com/esote/fuzz */

#define FAILS	100
#define HOST	"localhost"
#define PORT	"8441"

#define INPUT_L	12
static struct input {
	char *s;
	size_t n;
} inputs[] = {
	{ "[1,10,0]",				1e3 },
	{ "[1,65535,1]",			1e3 },
	{ "GET[1]",				1e3 },
	{ "GET [1]",				1e3 },
	{ "GET /[1]",				1e3 },
	{ "GET /[2]",				1e3 },
	{ "GET / HTTP[1]",			1e3 },
	{ "GET / HTTP/1.1\n[1]",		1e3 },
	{ "GET / HTTP/1.1\nHost: [1]\n\n",	1e3 },
	{ "GET / HTTP/1.1\nHost: [1]localhost\n\n", 1e3 },
	{ "GET / HTTP/1.1\nHost: localhost:[2]\n\n", 1e3 },
	{ "GET / HTTP/1.1\nHost: localhost:8441\n\n", 1e3 },
};

static int fails;

void
run(char *input, size_t n, size_t i, struct addrinfo *r)
{
	char *s;
	struct fuzz *f;
	size_t w;
	int sfd;

	if ((f = fuzz_init(input, "[", "]", ',')) == NULL) {
		err(1, "fuzz_init");
	}

	for (; n > 0; n--) {
		if ((s = fuzz(f, &w)) == NULL) {
			err(1, "fuzz");
		}

		if ((sfd = socket(r->ai_family, r->ai_socktype, r->ai_protocol)) == -1) {
			err(1, "socket");
		}

		if (connect(sfd, r->ai_addr, r->ai_addrlen) == -1) {
			if (++fails == FAILS) {
				err(1, "connect");
			}
			goto done;
		}

		if (write(sfd, s, w) == -1) {
			if (++fails == FAILS) {
				err(1, "write");
			}
		}

done:
		if (close(sfd) == -1) {
			err(1, "close");
		}
	}

	fuzz_free(f);

	(void)printf("%zu/%d\n", i + 1, INPUT_L);
}

int
main(void)
{
	struct sigaction act;
	struct addrinfo hints;
	struct addrinfo *result, *rp;
	size_t i;
	int s;

	(void)memset(&act, 0, sizeof(act));

	if (sigemptyset(&act.sa_mask) == -1) {
		err(1, "sigemptyset");
	}

	act.sa_handler = SIG_IGN;

	if (sigaction(SIGPIPE, &act, NULL) == -1) {
		err(1, "sigaction");
	}

	(void)memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((s = getaddrinfo(HOST, PORT, &hints, &result)) != 0) {
		errx(1, "getaddrinfo: %s", gai_strerror(s));
	}

	for (rp = result; rp != NULL; rp = rp->ai_next) {
		if ((s = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol)) == -1) {
			err(1, "socket");
		}

		if (connect(s, rp->ai_addr, rp->ai_addrlen) != -1) {
			(void)close(s);
			break;
		}

		if (close(s) == -1) {
			err(1, "close");
		}
	}

	if (rp == NULL) {
		errx(1, "failed to connect to %s:%s", HOST, PORT);
	}

	fails = 0;

	for (i = 0; i < INPUT_L; i++) {
		run(inputs[i].s, inputs[i].n, i, rp);
	}

	freeaddrinfo(result);

}
