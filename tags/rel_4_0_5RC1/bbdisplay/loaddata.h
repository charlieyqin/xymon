/*----------------------------------------------------------------------------*/
/* Hobbit overview webpage generator tool.                                    */
/*                                                                            */
/* Copyright (C) 2002-2005 Henrik Storner <henrik@storner.dk>                 */
/*                                                                            */
/* This program is released under the GNU General Public License (GPL),       */
/* version 2. See the file "COPYING" for details.                             */
/*                                                                            */
/*----------------------------------------------------------------------------*/

#ifndef __LOADDATA_H_
#define __LOADDATA_H_

extern int statuscount;

extern char     *ignorecolumns;
extern char	*dialupskin;
extern char	*reverseskin;
extern time_t   recentgif_limit;

extern int 	purplecount;
extern char 	*purplelogfn;

extern state_t *load_state(dispsummary_t **sumhead);

#endif
