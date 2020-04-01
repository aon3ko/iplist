#!/bin/bash
execpath=/etc/iplist
conf=autoadd.conf

if [ -z "$(command -v ipset)" ]; then
	echo "ipset not installed or not in \$PATH."
	exit
fi

while [ -n "$1" ]; do
	case $1 in
		*)
		echo "\
usage:  iplist-addipset.sh [config file location]
	default location at /etc/iplist/autoadd.conf
	*** this func not implemented ***"
		exit
		;;
	esac
done

CheckFlush() {
	ari=0
	for setName in $flushedSets; do
		[ $setName == $1 ] && ari=1
	done
	if [ $ari != "1" ]; then
		ipset flush $1
		flushedSets="$flushedSets $1"
	fi
}

while IFS=' ' read iplistFile iplistName Type; do
	if [ -f $execpath/inet/$iplistFile ]; then
		echo -n "adding $iplistName IPv4 part..."
		if [ -n "$Type" ]; then
			ipset -! create $iplistName-4 hash:$Type family inet
		else
			ipset -! create $iplistName-4 hash:net family inet
		fi
		CheckFlush $iplistName-4
		while read iprange; do
			ipset add $iplistName-4 $iprange
		done < $execpath/inet/$iplistFile
		echo "done."
	fi
	if [ -f $execpath/inet6/$iplistFile ]; then
		echo -n "adding $iplistName IPv6 part..."
		if [ -n "$Type" ]; then
			ipset -! create $iplistName-6 hash:$Type family inet6
		else
			ipset -! create $iplistName-6 hash:net family inet6
		fi
		CheckFlush $iplistName-6
		while read iprange; do
			ipset add $iplistName-6 $iprange
		done < $execpath/inet6/$iplistFile
		echo "done."
	fi
done < $execpath/$conf
