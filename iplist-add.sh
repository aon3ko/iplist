#!/bin/bash

execpath=/etc/iplist
conffile=autoadd.conf

[ -z "$(command -v ipset)" ] && echo "ipset not installed or not in \$PATH." && exit

while [ -n "$1" ]; do
	case $1 in
		-c)
			[ -z "$(command -v dirname)" ] && echo "dirname not installed or not in \$PATH." && exit
			[ -z "$(command -v basename)" ] && echo "basename not installed or not in \$PATH." && exit
			shift
			execpath="$(dirname $1)"
			conffile="$(basename $1)"
			;;
		*)
			echo "\
usage:  iplist-addipset.sh [config file location]
	default location at /etc/iplist/autoadd.conf"
			exit
			;;
	esac
	shift
done

[ ! -f $execpath/$conffile ] && echo "$execpath/$conffile does not exist." && exit

checkFlush() {
	local ari=0
	for setName in $flushedSets; do
		[ "$setName" == "$1" ] && ari=1 && break
	done
	if [ "$ari" != "1" ]; then
		ipset flush $1-4
		ipset flush $1-6
		flushedSets="$flushedSets $1"
	fi
}

while read iplistFile iplistName iplistType; do
	[ -z "$iplistFile" ] && continue
	[ -z "$iplistType" ] && iplistType="net"
	ipset -! create $iplistName-4 hash:$iplistType family inet
	ipset -! create $iplistName-6 hash:$iplistType family inet6
	checkFlush $iplistName
	IPvX=4

	echo -n "adding $iplistFile ..."
	while read; do
		[ -z "$REPLY" ] || ipset -! add $iplistName-$IPvX $REPLY 2> /dev/null && continue
		grep : <<< $REPLY > /dev/null && IPvX=6 || IPvX=4
		ipset -! add $iplistName-$IPvX $REPLY 2> /dev/null || echo "$REPLY cannot be add"
	done < $execpath/db/$iplistFile
	echo " done."
done < $execpath/$conffile
