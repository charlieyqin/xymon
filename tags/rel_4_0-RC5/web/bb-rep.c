/*----------------------------------------------------------------------------*/
/* Big Brother webpage generator tool.                                        */
/*                                                                            */
/* This is a replacement for the "bb-rep.sh" script                           */
/*                                                                            */
/* Primary reason for doing this: Shell scripts perform badly, and with a     */
/* medium-sized installation (~150 hosts) it takes several minutes to         */
/* generate the webpages. This is a problem, when the pages are used for      */
/* 24x7 monitoring of the system status.                                      */
/*                                                                            */
/* Copyright (C) 2003 Henrik Storner <henrik@storner.dk>                      */
/*                                                                            */
/* This program is released under the GNU General Public License (GPL),       */
/* version 2. See the file "COPYING" for details.                             */
/*                                                                            */
/*----------------------------------------------------------------------------*/

static char rcsid[] = "$Id: bb-rep.c,v 1.26 2005-01-20 10:45:44 henrik Exp $";

#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <time.h>
#include <signal.h>
#include <fcntl.h>

#include "bbgen.h"

/*
 * This program is invoked via CGI with QUERY_STRING containing:
 *
 *	start-mon=Jun&
 *	start-day=19&
 *	start-yr=2003&
 *	end-mon=Jun&
 *	end-day=19&
 *	end-yr=2003&
 *	style=crit&
 *	suburl=path&
 *	SUB=Generate+Report
 *
 */

char *reqenv[] = {
"BBHOME",
"BBREP",
"BBREPURL",
NULL };

char *style = "";
time_t starttime = 0;
time_t endtime = 0;
char *suburl = "";

char *monthnames[13] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec", NULL };

void errormsg(char *msg)
{
	printf("Content-type: text/html\n\n");
	printf("<html><head><title>Invalid request</title></head>\n");
	printf("<body>%s</body></html>\n", msg);
	exit(1);
}

void parse_query(void)
{
	char *query, *token;
	int startday, startmon, startyear;
	int endday, endmon, endyear;
	struct tm tmbuf;

	startday = startmon = startyear = endday = endmon = endyear = -1;

	if (xgetenv("QUERY_STRING") == NULL) {
		errormsg("Invalid request");
		return;
	}
	else query = urldecode("QUERY_STRING");

	if (!urlvalidate(query, NULL)) {
		errormsg("Invalid request");
		return;
	}

	token = strtok(query, "&");
	while (token) {
		char *val;
		
		val = strchr(token, '='); if (val) { *val = '\0'; val++; }

		if (argnmatch(token, "start-day")) {
			startday = atoi(val);
		}
		else if (argnmatch(token, "start-mon")) {
			char *errptr;

			startmon = strtol(val, &errptr, 10) - 1;
			if (errptr == val) {
				for (startmon=0; (monthnames[startmon] && strcmp(val, monthnames[startmon])); startmon++) ;
				if (startmon >= 12) startmon = -1;
			}
		}
		else if (argnmatch(token, "start-yr")) {
			startyear = atoi(val);
		}
		else if (argnmatch(token, "end-day")) {
			endday = atoi(val);
		}
		else if (argnmatch(token, "end-mon")) {
			char *errptr;

			endmon = strtol(val, &errptr, 10) - 1;
			if (errptr == val) {
				for (endmon=0; (monthnames[endmon] && strcmp(val, monthnames[endmon])); endmon++) ;
				if (endmon > 12) endmon = -1;
			}
		}
		else if (argnmatch(token, "end-yr")) {
			endyear = atoi(val);
		}
		else if (argnmatch(token, "style")) {
			style = strdup(val);
		}
		else if (argnmatch(token, "suburl")) {
			suburl = strdup(val);
		}

		token = strtok(NULL, "&");
	}

	memset(&tmbuf, 0, sizeof(tmbuf));
	tmbuf.tm_mday = startday;
	tmbuf.tm_mon = startmon;
	tmbuf.tm_year = startyear - 1900;
	tmbuf.tm_hour = 0;
	tmbuf.tm_min = 0;
	tmbuf.tm_sec = 0;
	tmbuf.tm_isdst = -1;		/* Important! Or we mishandle DST periods */
	starttime = mktime(&tmbuf);

	memset(&tmbuf, 0, sizeof(tmbuf));
	tmbuf.tm_mday = endday;
	tmbuf.tm_mon = endmon;
	tmbuf.tm_year = endyear - 1900;
	tmbuf.tm_hour = 23;
	tmbuf.tm_min = 59;
	tmbuf.tm_sec = 59;
	tmbuf.tm_isdst = -1;		/* Important! Or we mishandle DST periods */
	endtime = mktime(&tmbuf);

	if ((starttime == -1) || (endtime == -1) || (starttime > time(NULL))) errormsg("Invalid parameters");

	if (endtime > time(NULL)) endtime = time(NULL);

	if (starttime > endtime) {
		/* Swap start and end times */
		time_t tmp;

		tmp = endtime;
		endtime = starttime;
		starttime = tmp;
	}

	xfree(query);
}


void cleandir(char *dirname)
{
	DIR *dir;
	struct dirent *d;
	struct stat st;
	char fn[PATH_MAX];
	time_t killtime = time(NULL)-86400;

	dir = opendir(dirname);
	if (dir == NULL) return;

	while ((d = readdir(dir))) {
		if (d->d_name[0] != '.') {
			sprintf(fn, "%s/%s", dirname, d->d_name);
			if ((stat(fn, &st) == 0) && (st.st_mtime < killtime)) {
				if (S_ISREG(st.st_mode) || S_ISLNK(st.st_mode)) {
					dprintf("rm %s\n", fn);
					unlink(fn);
				}
				else if (S_ISDIR(st.st_mode)) {
					dprintf("Cleaning directory %s\n", fn);
					cleandir(fn);
					dprintf("rmdir %s\n", fn);
					rmdir(fn);
				}
				else { /* Ignore file */ };
			}
		}
	}
}


int main(int argc, char *argv[])
{
	char dirid[PATH_MAX];
	char outdir[PATH_MAX];
	char bbwebenv[PATH_MAX];
	char bbgencmd[PATH_MAX];
	char bbgentimeopt[100];
	char *bbgen_argv[20];
	pid_t childpid;
	int childstat;
	char htmldelim[20];
	char startstr[20], endstr[20];
	int cleanupoldreps = 1;
	int argi, newargi;

	newargi = 0;
	bbgen_argv[newargi++] = bbgencmd;
	bbgen_argv[newargi++] = bbgentimeopt;

	for (argi=1; (argi < argc); argi++) {
		if (argnmatch(argv[argi], "--env=")) {
			char *p = strchr(argv[argi], '=');
			loadenv(p+1);
		}
		else if (strcmp(argv[1], "--noclean") == 0) {
			cleanupoldreps = 0;
		}
		else {
			bbgen_argv[newargi++] = argv[argi];
		}
	}
	bbgen_argv[newargi++] = outdir;
	bbgen_argv[newargi++] = NULL;

	if ((xgetenv("QUERY_STRING") == NULL) || (strlen(xgetenv("QUERY_STRING")) == 0)) {
		/* Present the query form */
		int formfile;
		char formfn[PATH_MAX];

		sprintf(formfn, "%s/web/report_form", xgetenv("BBHOME"));
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

			headfoot(stdout, "report", "", "header", COL_BLUE);
			output_parsed(stdout, inbuf, COL_BLUE, "report");
			headfoot(stdout, "report", "", "footer", COL_BLUE);

			xfree(inbuf);
		}
		return 0;
	}

	envcheck(reqenv);
	parse_query();

	/*
	 * We need to set these variables up AFTER we have put them into the bbgen_argv[] array.
	 * We cannot do it before, because we need the environment that the the commandline options 
	 * might provide.
	 */
	if (xgetenv("BBGEN")) sprintf(bbgencmd, "%s", xgetenv("BBGEN"));
	else sprintf(bbgencmd, "%s/bin/bbgen", xgetenv("BBHOME"));

	sprintf(bbgentimeopt, "--reportopts=%u:%u:1:%s", (unsigned int)starttime, (unsigned int)endtime, style);

	sprintf(dirid, "%u-%u", (unsigned int)getpid(), (unsigned int)time(NULL));
	sprintf(outdir, "%s/%s", xgetenv("BBREP"), dirid);
	mkdir(outdir, 0755);


	sprintf(bbwebenv, "BBWEB=%s/%s", xgetenv("BBREPURL"), dirid);
	putenv(bbwebenv);

	/* Output the "please wait for report ... " thing */
	sprintf(htmldelim, "bbrep-%u-%u", (int)getpid(), (unsigned int)time(NULL));
	printf("Content-type: multipart/mixed;boundary=%s\n", htmldelim);
	printf("\n");
	printf("--%s\n", htmldelim);
	printf("Content-type: text/html\n\n");

	/* It's ok with these hardcoded values, as they are not used for this page */
	sethostenv("", "", "", colorname(COL_BLUE));
	sethostenv_report(starttime, endtime, 97.0, 99.995);
	headfoot(stdout, "bbrep", "", "header", COL_BLUE);

	strftime(startstr, sizeof(startstr), "%b %d %Y", localtime(&starttime));
	strftime(endstr, sizeof(endstr), "%b %d %Y", localtime(&endtime));
	printf("<CENTER><A NAME=begindata>&nbsp;</A>\n");
	printf("<BR><BR><BR><BR>\n");
	printf("<H3>Generating report for the period: %s - %s (%s)<BR>\n", startstr, endstr, style);
	printf("<P><P>\n");
	fflush(stdout);

	/* Go do the report */
	childpid = fork();
	if (childpid == 0) {
		execv(bbgencmd, bbgen_argv);
	}
	else if (childpid > 0) {
		wait(&childstat);

		/* Ignore SIGHUP so we dont get killed during cleanup of BBREP */
		signal(SIGHUP, SIG_IGN);

		if (WIFEXITED(childstat) && (WEXITSTATUS(childstat) != 0) ) {
			char msg[4096];

			printf("--%s\n\n", htmldelim);
			sprintf(msg, "Could not generate report.<br>\nCheck that the %s/www/rep/ directory has permissions '-rwxrwxr-x' (775)<br>\n and that is is set to group %d", xgetenv("BBHOME"), (int)getgid());
			errormsg(msg);
		}
		else {
			/* Send the browser off to the report */
			printf("Done...Report is <A HREF=\"%s/%s/%s\">here</a>.</P></BODY></HTML>\n", xgetenv("BBREPURL"), dirid, suburl);
			fflush(stdout);
			printf("--%s\n\n", htmldelim);
			printf("Content-Type: text/html\n\n");
			printf("<HTML><HEAD>\n");
			printf("<META HTTP-EQUIV=\"REFRESH\" CONTENT=\"0; URL=%s/%s/%s\"\n", 
					xgetenv("BBREPURL"), dirid, suburl);
			printf("</HEAD><BODY BGCOLOR=\"000000\"></BODY></HTML>\n");
			printf("\n--%s\n", htmldelim);
			fflush(stdout);
		}

		if (cleanupoldreps) cleandir(xgetenv("BBREP"));
	}
	else {
		printf("--%s\n\n", htmldelim);
		printf("Content-Type: text/html\n\n");
		errormsg("Fork failed");
	}

	return 0;
}

