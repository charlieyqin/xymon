/*----------------------------------------------------------------------------*/
/* bbgen tool                                                                 */
/*                                                                            */
/* Copyright (C) 2003 Henrik Storner <henrik@hswn.dk>                         */
/*                                                                            */
/* This program is released under the GNU General Public License (GPL),       */
/* version 2. See the file "COPYING" for details.                             */
/*                                                                            */
/*----------------------------------------------------------------------------*/

#ifndef __SENDMSG_H_
#define __SENDMSG_H_

#define BBDPORTNUMBER 1984
#define PAGELEVELSDEFAULT "red purple"

#define BB_OK			0
#define BB_EBADIP		1
#define BB_EIPUNKNOWN		2
#define BB_ENOSOCKET 		3
#define BB_ECANNOTDONONBLOCK	4
#define BB_ECONNFAILED		5
#define BB_ESELFAILED		6
#define BB_ETIMEOUT		7
#define BB_EWRITEERROR		8

extern int bbmsgcount;
extern int bbstatuscount;
extern int bbnocombocount;

extern int sendstatus(char *bbdisp, char *msg);
extern int sendmessage(char *msg, char *recipient);

extern void combo_start(void);
extern void combo_end(void);

extern void init_status(int color);
extern void addtostatus(char *p);
extern void finish_status(void);

#endif

