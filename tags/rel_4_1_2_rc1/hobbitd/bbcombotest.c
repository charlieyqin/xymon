/*----------------------------------------------------------------------------*/
/* Hobbit combination test tool.                                              */
/*                                                                            */
/* Copyright (C) 2003-2005 Henrik Storner <henrik@hswn.dk>                    */
/*                                                                            */
/* This program is released under the GNU General Public License (GPL),       */
/* version 2. See the file "COPYING" for details.                             */
/*                                                                            */
/*----------------------------------------------------------------------------*/

static char rcsid[] = "$Id: bbcombotest.c,v 1.40 2005-07-16 09:48:35 henrik Exp $";

#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

#include "version.h"
#include "libbbgen.h"

typedef struct value_t {
	char *symbol;
	int color;
	struct value_t *next;
} value_t;

typedef struct testspec_t {
	char *reshostname;
	char *restestname;
	char *expression;
	char *comment;
	char *resultexpr;
	value_t *valuelist;
	long result;
	char *errbuf;
	struct testspec_t *next;
} testspec_t;

static testspec_t *testhead = NULL;
static int testcount = 0;
static int cleanexpr = 0;

static char *gethname(char *spec)
{
	static char result[MAX_LINE_LEN];
	char *p;

	/* grab the hostname part from a "www.xxx.com.testname" string */
	strcpy(result, spec);
	p = strrchr(result, '.');
	if (p) *p = '\0';
	return result;
}

static char *gettname(char *spec)
{
	static char result[MAX_LINE_LEN];
	char *p;

	result[0] = '\0';

	/* grab the testname part from a "www.xxx.com.testname" string */
	p = strrchr(spec, '.');
	if (p) strcpy(result, p+1);

	return result;
}

static void loadtests(void)
{
	FILE *fd;
	char fn[PATH_MAX];
	char *inbuf = NULL;
	int inbufsz;

	sprintf(fn, "%s/etc/bbcombotest.cfg", xgetenv("BBHOME"));
	fd = fopen(fn, "r");
	if (fd == NULL) {
		/* 
		 * Why this ? Because I goofed and released a version using bbcombotests.cfg,
		 * and you shouldn't break peoples' setups when fixing silly bugs.
		 */
		sprintf(fn, "%s/etc/bbcombotests.cfg", xgetenv("BBHOME"));
		fd = fopen(fn, "r");
	}
	if (fd == NULL) {
		errprintf("Cannot open %s/etc/bbcombotest.cfg\n", xgetenv("BBHOME"));
		return;
	}

	initfgets(fd);
	while (unlimfgets(&inbuf, &inbufsz, fd)) {
		char *p, *comment;
		char *inp, *outp;

		p = strchr(inbuf, '\n'); if (p) *p = '\0';
		/* Strip whitespace */
		for (inp=outp=inbuf; ((*inp >= ' ') && (*inp != '#')); inp++) {
			if (isspace((int)*inp)) {
			}
			else {
				*outp = *inp;
				outp++;
			}
		}
		*outp = '\0';
		if (strlen(inp)) memmove(outp, inp, strlen(inp)+1);

		if (strlen(inbuf) && (inbuf[0] != '#') && (p = strchr(inbuf, '=')) ) {
			testspec_t *newtest = (testspec_t *) malloc(sizeof(testspec_t));

			*p = '\0';
			comment = strchr(p+1, '#');
			if (comment) *comment = '\0';
			newtest->reshostname = strdup(gethname(inbuf));
			newtest->restestname = strdup(gettname(inbuf));
			newtest->expression = strdup(p+1);
			newtest->comment = (comment ? strdup(comment+1) : NULL);
			newtest->resultexpr = NULL;
			newtest->valuelist = NULL;
			newtest->result = -1;
			newtest->errbuf = NULL;
			newtest->next = testhead;
			testhead = newtest;
			testcount++;
		}
	}

	fclose(fd);
	if (inbuf) xfree(inbuf);
}

static int gethobbitdvalue(char *hostname, char *testname, char **errptr)
{
	static char *board = NULL;
	int hobbitdresult;
	int result = COL_CLEAR;
	char *pattern, *found, *colstr;

	if (board == NULL) {
		hobbitdresult = sendmessage("hobbitdboard fields=hostname,testname,color", NULL, NULL, &board, 1, 30);
		if ((hobbitdresult != BB_OK) || (board == NULL)) {
			board = "";
			*errptr += sprintf(*errptr, "Could not access hobbitd board, error %d\n", hobbitdresult);
			return COL_CLEAR;
		}
	}

	pattern = (char *)malloc(1 + strlen(hostname) + 1 + strlen(testname) + 1 + 1);
	sprintf(pattern, "\n%s|%s|", hostname, testname);

	if (strncmp(board, pattern+1, strlen(pattern+1)) == 0) {
		/* The first entry in the board doesn't have the "\n" */
		found = board;
	}
	else {
		found = strstr(board, pattern);
	}

	if (found) {
		/* hostname|testname|color */
		colstr = found + strlen(pattern);
		result = parse_color(colstr);
	}

	xfree(pattern);
	return result;
}

static long getvalue(char *hostname, char *testname, int *color, char **errbuf)
{
	testspec_t *walk;
	char errtext[1024];
	char *errptr;

	*color = -1;
	errptr = errtext; 
	*errptr = '\0';

	/* First check if it is one of our own tests */
	for (walk = testhead; (walk && ( (strcmp(walk->reshostname, hostname) != 0) || (strcmp(walk->restestname, testname) != 0) ) ); walk = walk->next);
	if (walk != NULL) {
		/* It is a combo test they want the result of. */
		return walk->result;
	}

	*color = gethobbitdvalue(hostname, testname, &errptr);

	/* Save error messages */
	if (strlen(errtext) > 0) {
		if (*errbuf == NULL)
			*errbuf = strdup(errtext);
		else {
			*errbuf = (char *)realloc(*errbuf, strlen(*errbuf)+strlen(errtext)+1);
			strcat(*errbuf, errtext);
		}
	}

	if (*color == -1) return -1;
	else return ( (*color == COL_GREEN) || (*color == COL_YELLOW) || (*color == COL_CLEAR) );
}


static long evaluate(char *symbolicexpr, char **resultexpr, value_t **valuelist, char **errbuf)
{
	char expr[MAX_LINE_LEN];
	char *inp, *outp, *symp;
	char symbol[MAX_LINE_LEN];
	int done;
	int insymbol = 0;
	int result, error;
	long oneval;
	int onecolor;
	value_t *valhead = NULL, *valtail = NULL;
	value_t *newval;
	char errtext[1024];

	done = 0; inp=symbolicexpr; outp=expr; symp = NULL; 
	while (!done) {
		if (isalpha((int)*inp)) {
			if (!insymbol) { insymbol = 1; symp = symbol; }
			*symp = *inp; symp++;
		}
		else if (insymbol && (isdigit((int) *inp) || (*inp == '.'))) {
			*symp = *inp; symp++;
		}
		else if (insymbol && ((*inp == '\\') && (*(inp+1) > ' '))) {
			*symp = *(inp+1); symp++; inp++;
		}
		else {
			if (insymbol) {
				/* Symbol finished - evaluate the symbol */
				*symp = '\0';
				insymbol = 0;
				oneval = getvalue(gethname(symbol), gettname(symbol), &onecolor, errbuf);
				sprintf(outp, "%ld", oneval);
				outp += strlen(outp);

				newval = (value_t *) malloc(sizeof(value_t));
				newval->symbol = strdup(symbol);
				newval->color = onecolor;
				newval->next = NULL;
				if (valhead == NULL) {
					valtail = valhead = newval;
				}	
				else {
					valtail->next = newval;
					valtail = newval;
				}
			}

			*outp = *inp; outp++; symp = NULL;
		}

		if (*inp == '\0') done = 1; else inp++;
	}

	*outp = '\0';

	if (resultexpr) *resultexpr = strdup(expr);
	dprintf("Symbolic '%s' converted to '%s'\n", symbolicexpr, expr);

	error = 0; 
	result = compute(expr, &error);

	if (error) {
		sprintf(errtext, "compute(%s) returned error %d\n", expr, error);
		if (*errbuf == NULL) {
			*errbuf = strdup(errtext);
		}
		else {
			*errbuf = (char *)realloc(*errbuf, strlen(*errbuf)+strlen(errtext)+1);
			strcat(*errbuf, errtext);
		}
	}

	*valuelist = valhead;
	return result;
}

char *reqenv[] = {
"BB",
"BBDISP",
"BBHOME",
"BBLOGS",
"BBTMP",
NULL };


char *printify(char *exp)
{
	static char result[MAX_LINE_LEN];
	char *inp, *outp;
	size_t n;

	if (!cleanexpr) {
		return exp;
	}

	inp = exp;
	outp = result;

	while (*inp) {
		n = strcspn(inp, "|&");
		memcpy(outp, inp, n);
		inp += n; outp += n;

		if (*inp == '|') { 
			inp++;
			if (*inp == '|') {
				inp++;
				strcpy(outp, " OR "); outp += 4; 
			}
			else {
				strcpy(outp, " bOR "); outp += 5; 
			}
		}
		else if (*inp == '&') { 
			inp++; 
			if (*inp == '&') {
				inp++;
				strcpy(outp, " AND "); outp += 5; 
			}
			else {
				strcpy(outp, " bAND "); outp += 6;
			}
		}
	}

	*outp = '\0';
	return result;
}

int main(int argc, char *argv[])
{
	testspec_t *t;
	int argi, pending;
	int showeval = 1;

	setup_signalhandler("bbcombotest");

	for (argi = 1; (argi < argc); argi++) {
		if ((strcmp(argv[argi], "--help") == 0)) {
			printf("bbcombotest version %s\n\n", VERSION);
			printf("Usage:\n%s [--quiet] [--clean] [--debug] [--no-update]\n", argv[0]);
			exit(0);
		}
		else if ((strcmp(argv[argi], "--version") == 0)) {
			printf("bbcombotest version %s\n", VERSION);
			exit(0);
		}
		else if ((strcmp(argv[argi], "--debug") == 0)) {
			debug = 1;
		}
		else if ((strcmp(argv[argi], "--no-update") == 0)) {
			dontsendmessages = 1;
		}
		else if ((strcmp(argv[argi], "--quiet") == 0)) {
			showeval = 0;
		}
		else if ((strcmp(argv[argi], "--clean") == 0)) {
			cleanexpr = 1;
		}
	}

	envcheck(reqenv);
	init_timestamp();
	loadtests();

	/*
	 * Loop over the tests to allow us "forward refs" in expressions.
	 * We continue for as long as progress is being made.
	 */
	do {
		pending = testcount;
		for (t=testhead; (t); t = t->next) {
			if (t->result == -1) {
				t->result = evaluate(t->expression, &t->resultexpr, &t->valuelist, &t->errbuf);
				if (t->result != -1) testcount--;
			}
		}

	} while (pending != testcount);

	combo_start();
	for (t=testhead; (t); t = t->next) {
		char msgline[MAX_LINE_LEN];
		int color;
		value_t *vwalk;

		color = (t->result ? COL_GREEN : COL_RED);
		init_status(color);
		sprintf(msgline, "status %s.%s %s %s\n\n", commafy(t->reshostname), t->restestname, colorname(color), timestamp);
		addtostatus(msgline);
		if (t->comment) { addtostatus(t->comment); addtostatus("\n\n"); }
		if (showeval) {
			addtostatus(printify(t->expression));
			addtostatus(" = ");
			addtostatus(printify(t->resultexpr));
			addtostatus(" = ");
			sprintf(msgline, "%ld\n", t->result);
			addtostatus(msgline);

			for (vwalk = t->valuelist; (vwalk); vwalk = vwalk->next) {
				sprintf(msgline, "&%s %s\n", colorname(vwalk->color), vwalk->symbol);
				addtostatus(msgline);
			}

			if (t->errbuf) {
				addtostatus("\nErrors occurred during evaluation:\n");
				addtostatus(t->errbuf);
			}
		}
		finish_status();
	}
	combo_end();

	return 0;
}

