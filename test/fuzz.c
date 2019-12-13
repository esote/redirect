#include <err.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <fuzz.h> /* see https://github.com/esote/fuzz */

#define BUF_SIZ	4096
#define HOST	"localhost"
#define PORT	"8441"

void
run_fuzz(char *input, size_t n, struct addrinfo *result)
{
	char buf[BUF_SIZ];
	char *s;
	struct fuzz *f;
	struct addrinfo *rp;
	size_t w;
	ssize_t i;
	int sfd;

	if ((f = fuzz_init(input, "[", "]", ',')) == NULL) {
		err(1, "fuzz_init");
	}

	for (; n > 0; n--) {
		if ((s = fuzz(f, &w)) == NULL) {
			err(1, "fuzz");
		}

		for (rp = result; rp != NULL; rp = rp->ai_next) {
			if ((sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol)) == -1) {
				continue;
			}

			if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1) {
				break;
			}

			(void)close(sfd);
		}

		if (rp == NULL) {
			errx(1, "failed to connect to %s:%s", HOST, PORT);
		}

		if (write(sfd, s, w) == -1) {
			warn("write");
			break;
		}

		if ((i = read(sfd, buf, BUF_SIZ - 1)) == -1) {
			warn("read");
			break;
		}

		buf[i] = '\0';

		if (close(sfd) == -1) {
			err(1, "close");
		}
	}

	fuzz_free(f);
}

int
main(void)
{
	struct sigaction act;
	struct addrinfo hints;
	struct addrinfo *result;
	struct input {
		char *s;
		size_t n;
	} inputs[] = {
		{ "[1,10,0]",				1e3 },
		{ "[1,65535,1]",			1e1 },
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
		{ NULL, 0 }
	};
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

	for (i = 0; inputs[i].s != NULL; i++) {
		puts(inputs[i].s);
		run_fuzz(inputs[i].s, inputs[i].n, result);
	}

	freeaddrinfo(result);

}
