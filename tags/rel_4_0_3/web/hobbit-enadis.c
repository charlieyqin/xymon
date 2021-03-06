/*----------------------------------------------------------------------------*/
/* Hobbit backend script for disabling/enabling tests.                        */
/*                                                                            */
/* Copyright (C) 2003-2005 Henrik Storner <henrik@storner.dk>                 */
/*                                                                            */
/* This program is released under the GNU General Public License (GPL),       */
/* version 2. See the file "COPYING" for details.                             */
/*                                                                            */
/*----------------------------------------------------------------------------*/

static char rcsid[] = "$Id: hobbit-enadis.c,v 1.13 2005-05-09 20:39:30 henrik Exp $";

#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>

#include "libbbgen.h"

enum { ACT_NONE, ACT_FILTER, ACT_ENABLE, ACT_DISABLE, ACT_SCHED_DISABLE, ACT_SCHED_CANCEL } action = ACT_NONE;
int hostcount = 0;
char **hostnames  = NULL;
int disablecount = 0;
char **disabletest = NULL;
int enablecount = 0;
char **enabletest = NULL;
int duration = 0;
int scale = 1;
char *disablemsg = "No reason given";
time_t schedtime = 0;
int cancelid = 0;
int preview = 0;

char *hostpattern = NULL;
char *pagepattern = NULL;
char *ippattern = NULL;

void errormsg(char *msg)
{
        printf("Content-type: text/html\n\n");
        printf("<html><head><title>Invalid request</title></head>\n");
        printf("<body>%s</body></html>\n", msg);
        exit(1);
}

void parse_cgi(void)
{
	cgidata_t *postdata, *pwalk;
	struct tm schedtm;

	memset(&schedtm, 0, sizeof(schedtm));
	postdata = cgi_request();
	if (cgi_method == CGI_GET) return;

	if (!postdata) {
		errormsg(cgi_error());
	}

	pwalk = postdata;
	while (pwalk) {
		/*
		 * When handling the "go", the "Disable now" and "Schedule disable"
		 * radio buttons mess things up. So ignore the "go" if we have seen a
		 * "filter" request already.
		 */
		if ((strcmp(pwalk->name, "go") == 0) && (action != ACT_FILTER)) {
			if      (strcasecmp(pwalk->value, "enable") == 0)           action = ACT_ENABLE;
			else if (strcasecmp(pwalk->value, "disable now") == 0)      action = ACT_DISABLE;
			else if (strcasecmp(pwalk->value, "schedule disable") == 0) action = ACT_SCHED_DISABLE;
			else if (strcasecmp(pwalk->value, "cancel") == 0)           action = ACT_SCHED_CANCEL;
			else if (strcasecmp(pwalk->value, "apply filters") == 0)    action = ACT_FILTER;
		}
		else if (strcmp(pwalk->name, "duration") == 0) {
			duration = atoi(pwalk->value);
		}
		else if (strcmp(pwalk->name, "scale") == 0) {
			scale = atoi(pwalk->value);
		}
		else if (strcmp(pwalk->name, "cause") == 0) {
			disablemsg = strdup(pwalk->value);
		}
		else if (strcmp(pwalk->name, "hostname") == 0) {
			if (hostnames == NULL) {
				hostnames = (char **)malloc(2 * sizeof(char *));
				hostnames[0] = strdup(pwalk->value);
				hostnames[1] = NULL;
				hostcount = 1;
			}
			else {
				hostnames = (char **)realloc(hostnames, (hostcount + 2) * sizeof(char *));
				hostnames[hostcount] = strdup(pwalk->value);
				hostnames[hostcount+1] = NULL;
				hostcount++;
			}
		}
		else if (strcmp(pwalk->name, "enabletest") == 0) {
			char *val = pwalk->value;

			if (strcmp(val, "ALL") == 0) val = "*";

			if (enabletest == NULL) {
				enabletest = (char **)malloc(2 * sizeof(char *));
				enabletest[0] = strdup(val);
				enabletest[1] = NULL;
				enablecount = 1;
			}
			else {
				enabletest = (char **)realloc(enabletest, (enablecount + 2) * sizeof(char *));
				enabletest[enablecount] = strdup(val);
				enabletest[enablecount+1] = NULL;
				enablecount++;
			}
		}
		else if (strcmp(pwalk->name, "disabletest") == 0) {
			char *val = pwalk->value;

			if (strcmp(val, "ALL") == 0) val = "*";

			if (disabletest == NULL) {
				disabletest = (char **)malloc(2 * sizeof(char *));
				disabletest[0] = strdup(val);
				disabletest[1] = NULL;
				disablecount = 1;
			}
			else {
				disabletest = (char **)realloc(disabletest, (disablecount + 2) * sizeof(char *));
				disabletest[disablecount] = strdup(val);
				disabletest[disablecount+1] = NULL;
				disablecount++;
			}
		}
		else if (strcmp(pwalk->name, "year") == 0) {
			schedtm.tm_year = atoi(pwalk->value) - 1900;
		}
		else if (strcmp(pwalk->name, "month") == 0) {
			schedtm.tm_mon = atoi(pwalk->value) - 1;
		}
		else if (strcmp(pwalk->name, "day") == 0) {
			schedtm.tm_mday = atoi(pwalk->value);
		}
		else if (strcmp(pwalk->name, "hour") == 0) {
			schedtm.tm_hour = atoi(pwalk->value);
		}
		else if (strcmp(pwalk->name, "minute") == 0) {
			schedtm.tm_min = atoi(pwalk->value);
		}
		else if (strcmp(pwalk->name, "canceljob") == 0) {
			cancelid = atoi(pwalk->value);
		}
		else if (strcmp(pwalk->name, "preview") == 0) {
			preview = (strcasecmp(pwalk->value, "on") == 0);
		}
		else if ((strcmp(pwalk->name, "hostpattern") == 0) && pwalk->value && strlen(pwalk->value)) {
			hostpattern = strdup(pwalk->value);
		}
		else if ((strcmp(pwalk->name, "pagepattern") == 0) && pwalk->value && strlen(pwalk->value)) {
			pagepattern = strdup(pwalk->value);
		}
		else if ((strcmp(pwalk->name, "ippattern") == 0)   && pwalk->value && strlen(pwalk->value)) {
			ippattern = strdup(pwalk->value);
		}

		pwalk = pwalk->next;
	}

	schedtm.tm_isdst = -1;
	schedtime = mktime(&schedtm);
}

void do_one_host(char *hostname, char *fullmsg, char *username)
{
	char hobbitcmd[4096];
	int i, result;

	switch (action) {
	  case ACT_ENABLE:
		for (i=0; (i < enablecount); i++) {
			if (preview) result = 0;
			else {
				sprintf(hobbitcmd, "enable %s.%s", commafy(hostname), enabletest[i]);
				result = sendmessage(hobbitcmd, NULL, NULL, NULL, 0, BBTALK_TIMEOUT);
				sprintf(hobbitcmd, "notify %s.%s\nMonitoring of %s:%s has been ENABLED by %s\n", 
					commafy(hostname), enabletest[i], 
					hostname, enabletest[i], username);
				sendmessage(hobbitcmd, NULL, NULL, NULL, 0, BBTALK_TIMEOUT);
			}
			printf("<tr><td>Enabling host <b>%s</b> test <b>%s</b> : %s</td></tr>\n", 
				hostname, enabletest[i], ((result == BB_OK) ? "OK" : "Failed"));
		}
		break;

	  case ACT_DISABLE:
		for (i=0; (i < disablecount); i++) {
			if (preview) result = 0;
			else {
				sprintf(hobbitcmd, "disable %s.%s %d %s", 
					commafy(hostname), disabletest[i], duration*scale, fullmsg);
				result = sendmessage(hobbitcmd, NULL, NULL, NULL, 0, BBTALK_TIMEOUT);
				sprintf(hobbitcmd, "notify %s.%s\nMonitoring of %s:%s has been DISABLED by %s for %d minutes\n%s", 
					commafy(hostname), disabletest[i], 
					hostname, disabletest[i], username, duration*scale, fullmsg);
				result = sendmessage(hobbitcmd, NULL, NULL, NULL, 0, BBTALK_TIMEOUT);
			}
			printf("<tr><td>Disabling host <b>%s</b> test <b>%s</b>: %s</td></tr>\n", 
				hostname, disabletest[i], ((result == BB_OK) ? "OK" : "Failed"));
		}
		break;

	  case ACT_SCHED_DISABLE:
		for (i=0; (i < disablecount); i++) {
			sprintf(hobbitcmd, "schedule %d disable %s.%s %d %s", 
				(int) schedtime, commafy(hostname), disabletest[i], duration*scale, fullmsg);
			result = (preview ? 0 : sendmessage(hobbitcmd, NULL, NULL, NULL, 0, BBTALK_TIMEOUT));
			printf("<tr><td>Scheduling disable of host <b>%s</b> test <b>%s</b> at <b>%s</b>: %s</td></tr>\n", 
				hostname, disabletest[i], ctime(&schedtime), ((result == BB_OK) ? "OK" : "Failed"));
		}
		break;

	  case ACT_SCHED_CANCEL:
		sprintf(hobbitcmd, "schedule cancel %d", cancelid);
		result = (preview ? 0 : sendmessage(hobbitcmd, NULL, NULL, NULL, 0, BBTALK_TIMEOUT));
		printf("<tr><td>Canceling job <b>%d</b> : %s</td></tr>\n", cancelid, ((result == BB_OK) ? "OK" : "Failed"));
		break;

	  default:
		errprintf("No action\n");
		break;
	}
}

int main(int argc, char *argv[])
{
	int argi, i;
	char *username = getenv("REMOTE_USER");
	char *userhost = getenv("REMOTE_HOST");
	char *userip   = getenv("REMOTE_ADDR");
	char *fullmsg = "No cause specified";
	char *envarea = NULL;

	if ((username == NULL) || (strlen(username) == 0)) username = "unknown";
	if ((userhost == NULL) || (strlen(userhost) == 0)) userhost = userip;
	
	for (argi=1; (argi < argc); argi++) {
		if (argnmatch(argv[argi], "--env=")) {
			char *p = strchr(argv[argi], '=');
			loadenv(p+1, envarea);
		}
		else if (argnmatch(argv[argi], "--area=")) {
			char *p = strchr(argv[argi], '=');
			envarea = strdup(p+1);
		}
		else if (strcmp(argv[argi], "--debug") == 0) {
			debug = 1;
		}
	}

	parse_cgi();
	if (debug) preview = 1;

	if (cgi_method == CGI_GET) {
		/*
		 * It's a GET , so the initial request.
		 * If we have a pagepath cookie, use that as the initial
		 * host-name filter.
		 */
		char *cookie, *p;

		action = ACT_FILTER;

		cookie = getenv("HTTP_COOKIE");
		if (cookie && ((p = strstr(cookie, "pagepath=")) != NULL)) {
			p += strlen("pagepath=");
			pagepattern = strdup(p);
			p = strchr(pagepattern, ';'); if (p) *p = '\0';
			if (strlen(pagepattern) == 0) { xfree(pagepattern); pagepattern = 0; }
			sethostenv_filter(NULL, pagepattern, NULL);
		}
	}

	if (action == ACT_FILTER) {
		/* Present the query form */
		int formfile;
		char formfn[PATH_MAX];

		sethostenv_filter(hostpattern, pagepattern, ippattern);

		load_hostnames(xgetenv("BBHOSTS"), NULL, get_fqdn());

		sprintf(formfn, "%s/web/maint_form", xgetenv("BBHOME"));
		formfile = open(formfn, O_RDONLY);

		if (formfile >= 0) {
			char *inbuf;
			struct stat st;

			fstat(formfile, &st);
			inbuf = (char *) malloc(st.st_size + 1);
			read(formfile, inbuf, st.st_size);
			inbuf[st.st_size] = '\0';
			close(formfile);

			printf("Content-Type: text/html\n\n");
			sethostenv("", "", "", colorname(COL_BLUE));

			headfoot(stdout, "maint", "", "header", COL_BLUE);
			output_parsed(stdout, inbuf, COL_BLUE, "report", time(NULL));
			headfoot(stdout, "maint", "", "footer", COL_BLUE);

			xfree(inbuf);
		}
		return 0;
	}

	fullmsg = (char *)malloc(strlen(username) + strlen(userhost) + strlen(disablemsg) + 1024);
	sprintf(fullmsg, "\nDisabled by: %s @ %s\nReason: %s\n", username, userhost, disablemsg);

	/*
	 * Ready ... go build the webpage.
	 */
	printf("Content-Type: text/html\n\n");
	printf("<html>\n");
	if (!preview) {
		printf("<head>\n<meta http-equiv=\"refresh\" content=\"3; URL=%s\"></head>\n", xgetenv("HTTP_REFERER"));
	}

        /* It's ok with these hardcoded values, as they are not used for this page */
	sethostenv("", "", "", colorname(COL_BLUE));
	headfoot(stdout, "maint", "", "header", COL_BLUE);

	if (debug) {
		printf("<pre>\n");
		switch (action) {
		  case ACT_NONE   : dprintf("Action = none\n"); break;

		  case ACT_FILTER : dprintf("Action = filter\n"); break;

		  case ACT_ENABLE : dprintf("Action = enable\n"); 
				    dprintf("Tests = ");
				    for (i=0; (i < enablecount); i++) printf("%s ", enabletest[i]);
				    printf("\n");
				    break;

		  case ACT_DISABLE: dprintf("Action = disable\n"); 
				    dprintf("Tests = ");
				    for (i=0; (i < disablecount); i++) printf("%s ", disabletest[i]);
				    printf("\n");
				    dprintf("Duration = %d, scale = %d\n", duration, scale);
				    dprintf("Cause = %s\n", textornull(disablemsg));
				    break;

		  case ACT_SCHED_DISABLE:
				    dprintf("Action = schedule\n");
				    dprintf("Time = %s\n", ctime(&schedtime));
				    dprintf("Tests = ");
				    for (i=0; (i < disablecount); i++) printf("%s ", disabletest[i]);
				    printf("\n");
				    dprintf("Duration = %d, scale = %d\n", duration, scale);
				    dprintf("Cause = %s\n", textornull(disablemsg));
				    break;

		  case ACT_SCHED_CANCEL:
				    dprintf("Action = cancel\n");
				    dprintf("ID = %d\n", cancelid);
				    break;
		}
		printf("</pre>\n");
	}

	printf("<table align=\"center\" summary=\"Actions performed\" width=\"60%%\">\n");
	if (action == ACT_SCHED_CANCEL) {
		do_one_host(NULL, NULL, username);
	}
	else {
		for (i = 0; (i < hostcount); i++) do_one_host(hostnames[i], fullmsg, username);
	}
	if (!preview) {
		printf("<tr><td><br>Please wait while refreshing status list ...</td></tr>\n");
	}
	else {
		printf("<tr><td align=center><br><br><form method=\"GET\" ACTION=\"%s\"><input type=submit value=\"Continue\"></form></td></tr>\n", xgetenv("HTTP_REFERER"));
	}
	printf("</table>\n");

	headfoot(stdout, "maint", "", "footer", COL_BLUE);

	return 0;
}

