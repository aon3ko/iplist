#!/bin/bash
execpath=/etc/iplist
execfile=autoadd.conf

if [ ! -z $1 ]; then
	echo "usage: autoadd-ipset.sh [config file location]\
	default location at /etc/iplist/autoadd.conf\
	not implement yet :)"
fi

while IFS=' ' read setfile setname; do
	if test -f $execpath/inet/$setfile; then
		echo "adding $setname IPv4 part."
		ipset create $setname-4 hash:net family inet
		ipset flush $setname-4
		while read iprange; do
			ipset add $setname-4 $iprange
		done < $execpath/inet/$setfile
		echo "adding $setname IPv4 done."
	fi
	if test -f $execpath/inet6/$setfile; then
		echo "adding $setname IPv6 part."
		ipset create $setname-6 hash:net family inet6
		ipset flush $setname-6
		while read iprange; do
			ipset add $setname-6 $iprange
		done < $execpath/inet6/$setfile
		echo "adding $setname IPv6 done."
	fi
done < $execpath/$execfile
