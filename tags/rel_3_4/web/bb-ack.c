/*----------------------------------------------------------------------------*/
/* Big Brother webpage generator tool.                                        */
/*                                                                            */
/* This is a replacement for the "bb-ack.sh" script                           */
/*                                                                            */
/* Copyright (C) 2004 Henrik Storner <henrik@storner.dk>                      */
/*                                                                            */
/* This program is released under the GNU General Public License (GPL),       */
/* version 2. See the file "COPYING" for details.                             */
/*                                                                            */
/*----------------------------------------------------------------------------*/

static char rcsid[] = "$Id: bb-ack.c,v 1.5 2004-11-21 12:59:49 henrik Exp $";

#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "libbbgen.h"
#include "version.h"

/*
 * This program is invoked via CGI with QUERY_STRING containing:
 *
 *      ACTION=action&NUMBER=acknum&DELAY=validity&MESSAGE=text
 */

char *reqenv[] = {
	"BBDISP",
	"BBHOME",
	NULL 
};

static char *action = "";
static int  acknum = 0;
static int  validity = 0;
static char *ackmsg = "";

static void errormsg(char *msg)
{
	printf("Content-type: text/html\n\n");
	printf("<html><head><title>Invalid request</title></head>\n");
	printf("<body>%s</body></html>\n", msg);
	exit(1);
}

static void parse_query(void)
{
	char *query, *token;

	if (getenv("QUERY_STRING") == NULL) {
		errormsg("Invalid request");
		return;
	}
	else query = urldecode("QUERY_STRING");

	token = strtok(query, "&");
	while (token) {
		char *val;
		val = strchr(token, '='); if (val) { *val = '\0'; val++; }
		if (argnmatch(token, "ACTION")) {
			action = strdup(val);
		}
		else if (argnmatch(token, "NUMBER")) {
			acknum = atoi(val);
		}
		else if (argnmatch(token, "DELAY")) {
			validity = atoi(val);
		}
		else if (argnmatch(token, "MESSAGE")) {
			ackmsg = strdup(val);
		}

		token = strtok(NULL, "&");
	}

        free(query);
}

int main(int argc, char *argv[])
{
	int argi, bbresult;
	char bbmsg[MAXMSG];
	char *respmsg = "";

	for (argi = 1; (argi < argc); argi++) {
		if (argnmatch(argv[argi], "--env=")) {
			char *p = strchr(argv[argi], '=');
			loadenv(p+1);
		}
		else if (strcmp(argv[argi], "--debug") == 0) {
			debug = 1;
		}
	}

	if ((getenv("QUERY_STRING") == NULL) || (strlen(getenv("QUERY_STRING")) == 0)) {
		/* Present the query form */
		int formfile;
		char formfn[PATH_MAX];

		sprintf(formfn, "%s/web/acknowledge_form", getenv("BBHOME"));
		formfile = open(formfn, O_RDONLY);

		if (formfile >= 0) {
			int n;
			char *inbuf;
			struct stat st;

			fstat(formfile, &st);
			inbuf = (char *) malloc(st.st_size + 1);
			read(formfile, inbuf, st.st_size);
			inbuf[st.st_size] = '\0';
			close(formfile);

			printf("Content-Type: text/html\n\n");
			sethostenv("", "", "", colorname(COL_RED));

			headfoot(stdout, "acknowledge", "", "header", COL_RED);
			output_parsed(stdout, inbuf, COL_RED, "acknowledge");
			headfoot(stdout, "acknowledge", "", "footer", COL_RED);

			free(inbuf);
		}
		return 0;
	}

	envcheck(reqenv);
	parse_query();

	if (strcasecmp(action, "ack") == 0) {
		sprintf(bbmsg, "ack ack_event %d %d %s", acknum, validity, ackmsg);
		bbresult = sendmessage(bbmsg, NULL, NULL, NULL, 0, 30);
		if (bbresult != BB_OK) {
			respmsg = "<center><h4>Could not contact BB servers</h4></center>\n";
		}
		else {
			respmsg = "<center><h4>Acknowledgment sent to BB servers</h4></center>\n";
		}
	}
	else if (strcasecmp(action, "page") == 0) {
		respmsg = "<center><h4>This system does not support paging the operator</h4></center>\n";
	}
	else {
		respmsg = "<center><h4>Unknown action ignored</h4></center>\n";
	}

	fprintf(stdout, "Content-type: text/html\n\n");
	
	headfoot(stdout, "acknowledge", "", "header", COL_RED);
	fprintf(stdout, "%s", respmsg);
	headfoot(stdout, "acknowledge", "", "footer", COL_RED);

	return 0;
}

