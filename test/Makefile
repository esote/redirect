CFLAGS=		-g -Wall -Wextra -Wconversion -Wundef
LDFLAGS=	-lfuzz

fuzz: fuzz.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o fuzz.out fuzz.c

parallel: parallel.c
	$(CC) $(CFLAGS) $(LDFLAGS) -pthread -o parallel.out parallel.c

clean:
	rm -f fuzz.out
