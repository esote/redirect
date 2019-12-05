#ifndef _REDIRECT_H
#define _REDIRECT_H

#define NL	"\r\n"
#define SP	" \t\v\f"

#define BUF_LEN		4096
#define PORT		8441
#define QUEUE_LEN	100
#define T_SEC		1

struct route {
	char *from;
	char *to;
};

struct route *	config(char *);
void		config_free(struct route *);
void		respond(int, struct route *);

#endif
