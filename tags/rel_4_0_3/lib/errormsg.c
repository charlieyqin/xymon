/*----------------------------------------------------------------------------*/
/* Hobbit monitor library.                                                    */
/*                                                                            */
/* This is a library module, part of libbbgen.                                */
/* It contains routines for error- and debug-message display.                 */
/*                                                                            */
/* Copyright (C) 2002-2005 Henrik Storner <henrik@storner.dk>                 */
/*                                                                            */
/* This program is released under the GNU General Public License (GPL),       */
/* version 2. See the file "COPYING" for details.                             */
/*                                                                            */
/*----------------------------------------------------------------------------*/

static char rcsid[] = "$Id: errormsg.c,v 1.7 2005-03-22 09:16:49 henrik Exp $";

#include <sys/types.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "libbbgen.h"

char *errbuf = NULL;
int save_errbuf = 1;
static unsigned int errbufsize = 0;

int debug = 0;
static FILE *tracefd = NULL;

void errprintf(const char *fmt, ...)
{
	char timestr[30];
	char msg[4096];
	va_list args;

	time_t now = time(NULL);

	MEMDEFINE(timestr);
	MEMDEFINE(msg);

	strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", localtime(&now));
	fprintf(stderr, "%s ", timestr);

	va_start(args, fmt);
#ifdef NO_VSNPRINTF
	vsprintf(msg, fmt, args);
#else
	vsnprintf(msg, sizeof(msg), fmt, args);
#endif
	va_end(args);

	fprintf(stderr, "%s", msg);
	fflush(stderr);

	if (save_errbuf) {
		if (errbuf == NULL) {
			errbufsize = 8192;
			errbuf = (char *) malloc(errbufsize);
			*errbuf = '\0';
		}
		else if ((strlen(errbuf) + strlen(msg)) > errbufsize) {
			errbufsize += 8192;
			errbuf = (char *) realloc(errbuf, errbufsize);
		}

		strcat(errbuf, msg);
	}

	MEMUNDEFINE(timestr);
	MEMUNDEFINE(msg);
}


void dprintf(const char *fmt, ...)
{
	va_list args;

	if (debug) {
		char timestr[30];
		time_t now = time(NULL);

		MEMDEFINE(timestr);

		strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S",
			 localtime(&now));
		printf("%s ", timestr);

		va_start(args, fmt);
		vprintf(fmt, args);
		va_end(args);
		fflush(stdout);

		MEMUNDEFINE(timestr);
	}
}

void flush_errbuf(void)
{
	if (errbuf) xfree(errbuf);
	errbuf = NULL;
}


void starttrace(const char *fn)
{
	if (tracefd) fclose(tracefd);
	if (fn) {
		tracefd = fopen(fn, "a"); 
		if (tracefd == NULL) errprintf("Cannot open tracefile %s\n", fn);
	}
	else tracefd = stdout;
}

void stoptrace(void)
{
	if (tracefd) fclose(tracefd);
	tracefd = NULL;
}

void traceprintf(const char *fmt, ...)
{
	va_list args;

	if (tracefd) {
		char timestr[40];
		time_t now = time(NULL);

		MEMDEFINE(timestr);

		strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", localtime(&now));
		fprintf(tracefd, "%08u %s ", (unsigned int)getpid(), timestr);

		va_start(args, fmt);
		vfprintf(tracefd, fmt, args);
		va_end(args);
		fflush(tracefd);

		MEMUNDEFINE(timestr);
	}
}

