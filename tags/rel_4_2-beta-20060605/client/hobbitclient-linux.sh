#!/bin/sh
#----------------------------------------------------------------------------#
# Linux client for Hobbit                                                    #
#                                                                            #
# Copyright (C) 2005-2006 Henrik Storner <henrik@hswn.dk>                    #
#                                                                            #
# This program is released under the GNU General Public License (GPL),       #
# version 2. See the file "COPYING" for details.                             #
#                                                                            #
#----------------------------------------------------------------------------#
#
# $Id: hobbitclient-linux.sh,v 1.16 2006-06-03 10:47:05 henrik Exp $

echo "[date]"
date
echo "[uname]"
uname -rsmn
echo "[osversion]"
if [ -x /bin/lsb_release ]; then
	/bin/lsb_release -r -i -s | xargs echo
	/bin/lsb_release -a
elif [ -f /etc/redhat-release ]; then
	cat /etc/redhat-release
elif [ -f /etc/gentoo-release ]; then
	cat /etc/gentoo-release
elif [ -f /etc/debian_version ]; then
	echo -en "Debian "
	cat /etc/debian_version
elif [ -f /etc/SuSE-release ]; then
	grep ^SuSE /etc/SuSE-release
fi
echo "[uptime]"
uptime
echo "[who]"
who
echo "[df]"
df -Pl -x none -x tmpfs -x shmfs -x unknown -x iso9660 | sed -e '/^[^ 	][^ 	]*$/{
N
s/[ 	]*\n[ 	]*/ /
}'
echo "[free]"
free
echo "[ifconfig]"
/sbin/ifconfig
echo "[route]"
netstat -rn
echo "[netstat]"
netstat -s
echo "[ports]"
# Bug in RedHat's netstat spews annoying error messages. 
netstat -ant 2>/dev/null
echo "[ifstat]"
/sbin/ifconfig
echo "[ps]"
ps -Aw -o pid,ppid,user,start,state,pri,pcpu,time,pmem,rsz,vsz,cmd
echo "[top]"
top -b -n 1 
# vmstat
nohup sh -c "vmstat 300 2 1>$BBTMP/hobbit_vmstat.$MACHINEDOTS.$$ 2>&1; mv $BBTMP/hobbit_vmstat.$MACHINEDOTS.$$ $BBTMP/hobbit_vmstat.$MACHINEDOTS" </dev/null >/dev/null 2>&1 &
sleep 5
if test -f $BBTMP/hobbit_vmstat.$MACHINEDOTS; then echo "[vmstat]"; cat $BBTMP/hobbit_vmstat.$MACHINEDOTS; rm -f $BBTMP/hobbit_vmstat.$MACHINEDOTS; fi
# logfiles
if test -f $LOGFETCHCFG
then
    $BBHOME/bin/logfetch $LOGFETCHCFG $LOGFETCHSTATUS
fi

exit

