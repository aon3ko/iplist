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

[ ! -f ${execpath}/${conffile} ] && echo "${execpath}/${conffile} does not exist." && exit

CheckFlush() {
	ari=0
	for setName in $flushedSets; do
		[ "$setName" == "$1" ] && ari=1 && break
	done
	if [ "$ari" != "1" ]; then
		ipset flush $1
		flushedSets="$flushedSets $1"
	fi
}

while read iplistFile iplistName Type; do
	[ -z "$iplistFile" ] && continue
	[ -z "$Type" ] && Type="net"

	if [ -f ${execpath}/inet/${iplistFile} ]; then
		echo -n "adding ${iplistFile} IPv4 part to ${iplistName}-4..."
		ipset -! create ${iplistName}-4 hash:$Type family inet
		CheckFlush ${iplistName}-4
		while read iprange; do
			[ -n "$iprange" ] && ipset add ${iplistName}-4 $iprange
		done < ${execpath}/inet/${iplistFile}
		echo "done."
	fi
	if [ -f ${execpath}/inet6/${iplistFile} ]; then
		echo -n "adding ${iplistFile} IPv6 part to ${iplistName}-6..."
		ipset -! create ${iplistName}-6 hash:$Type family inet6
		CheckFlush ${iplistName}-6
		while read iprange; do
			[ -n "$iprange" ] && ipset add ${iplistName}-6 $iprange
		done < ${execpath}/inet6/${iplistFile}
		echo "done."
	fi
done < ${execpath}/${conffile}
