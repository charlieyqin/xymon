/*----------------------------------------------------------------------------*/
/* Big Brother webpage generator tool.                                        */
/*                                                                            */
/* This is a replacement for the "mkbb.sh" and "mkbb2.sh" scripts from the    */
/* "Big Brother" monitoring tool from BB4 Technologies.                       */
/*                                                                            */
/* Primary reason for doing this: Shell scripts perform badly, and with a     */
/* medium-sized installation (~150 hosts) it takes several minutes to         */
/* generate the webpages. This is a problem, when the pages are used for      */
/* 24x7 monitoring of the system status.                                      */
/*                                                                            */
/* Copyright (C) 2002 Henrik Storner <henrik@storner.dk>                      */
/*                                                                            */
/* This program is released under the GNU General Public License (GPL),       */
/* version 2. See the file "COPYING" for details.                             */
/*                                                                            */
/*----------------------------------------------------------------------------*/

#ifndef __UTIL_H_
#define __UTIL_H_

extern int use_recentgifs;
extern char timestamp[];

extern FILE *stackfopen(char *filename, char *mode);
extern int stackfclose(FILE *fd);
extern char *stackfgets(char *buffer, unsigned int bufferlen, char *includetag);

extern char *malcop(const char *s);
extern void init_timestamp(void);

extern char *colorname(int color);
extern int eventcolor(char *colortext);
extern char *dotgiffilename(int color, int acked, int oldage);
extern char *weekday_text(char *dayspec);
extern char *time_text(char *timespec);
extern char *alttag(entry_t *e);
extern char *hostpage_link(host_t *host);
extern char *hostpage_name(host_t *host);
extern char *commafy(char *hostname);
extern void sethostenv(char *host, char *ip, char *svc, char *color);
extern void headfoot(FILE *output, char *pagetype, char *pagepath, char *head_or_foot, int bgcolor);
extern int checkalert(host_t *host, char *test);
extern int checkpropagation(host_t *host, char *test, int color);

extern link_t *find_link(const char *name);
extern char *columnlink(link_t *link, char *colname);
extern char *hostlink(link_t *link);
extern char *cgidoclink(const char *doccgi, const char *hostname);

extern host_t *find_host(const char *hostname);
extern char *histlogurl(char *hostname, char *service, time_t histtime);

extern int within_sla(char *hostline);
extern int periodcoversnow(char *tag);

extern void combo_start(void);
extern void combo_end(void);

extern void init_status(int color);
extern void addtostatus(char *p);
extern void finish_status(void);

extern void envcheck(char *envvars[]);

extern int run_columngen(char *column, int update_interval, int enabled);
extern char *realurl(char *url, char **proxy);

extern int generate_static(void);
extern int stdout_on_file(char *filename);

#endif

