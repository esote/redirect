#include <err.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/*
Make a total of REQS requests of REQ to HOST:PORT, split over PL threads,
absorbing at most FAILS fails.
*/

#define FAILS	100
#define HOST	"localhost"
#define PL	10
#define PORT	"8441"
#define REQ	"GET /abc HTTP/1.1\nHost: localhost:8441\n\n"
#define REQS	1000

#define REQ_L	(sizeof(REQ) - 1)

struct arg {
	size_t i;
	struct addrinfo *r;
};

static pthread_mutex_t mutex;
static int fails;

#define LOCK()	do {				\
	if (pthread_mutex_lock(&mutex) != 0) {	\
		err(1, "pthread_mutex_lock");	\
	}					\
} while(0)

#define UNLOCK()	do {				\
	if (pthread_mutex_unlock(&mutex) != 0) {	\
		err(1, "pthread_mutex_unlock");		\
	}						\
} while(0)

#define CHECK(I, R)	do {			\
	LOCK();					\
	if (++fails == FAILS) {			\
		err(1, "t%zu: %s", I, R);	\
	}					\
	UNLOCK();				\
} while (0)

static void *
run(void *arg)
{
	struct arg *a;
	size_t i;
	int sfd;

	a = arg;

	for (i = 0; i < (REQS / PL); i++) {
		if ((sfd = socket(a->r->ai_family, a->r->ai_socktype, a->r->ai_protocol)) == -1) {
			err(1, "socket");
		}

		if (connect(sfd, a->r->ai_addr, a->r->ai_addrlen) == -1) {
			CHECK(a->i, "connect");
			goto done;
		}

		if (write(sfd, REQ, REQ_L) == -1) {
			CHECK(a->i, "write");
		}

done:
		if (close(sfd) == -1) {
			err(1, "close");
		}
	}

	LOCK();
	(void)printf("t%zu: done\n", a->i);
	UNLOCK();

	return NULL;
}

int
main(void)
{
	pthread_t threads[PL];
	struct arg args[PL];
	struct addrinfo hints;
	struct addrinfo *result, *rp;
	struct sigaction act;
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

	if (pthread_mutex_init(&mutex, NULL) != 0) {
		err(1, "pthread_mutex_init");
	}

	fails = 0;

	for (i = 0; i < PL; i++) {
		args[i].i = i;
		args[i].r = rp;
		if (pthread_create(&threads[i], NULL, run, &args[i]) != 0) {
			err(1, "pthread_create");
		}
	}

	for (i = 0; i < PL; i++) {
		if (pthread_join(threads[i], NULL) != 0) {
			err(1, "pthread_join");
		}
	}

	if (pthread_mutex_destroy(&mutex) != 0) {
		err(1, "pthread_mutex_destroy");
	}

	freeaddrinfo(result);
}
