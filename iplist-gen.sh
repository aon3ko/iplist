#!/bin/bash

execpath=/etc/iplist
conffile=autogen.conf

ARIN=https://ftp.arin.net/pub/stats/arin/delegated-arin-extended-latest	# only extended
APNIC=https://ftp.apnic.net/apnic/stats/apnic/delegated-apnic-latest
RIPENCC=https://ftp.ripe.net/pub/stats/ripencc/delegated-ripencc-latest
AFRINIC=https://ftp.afrinic.net/stats/afrinic/delegated-afrinic-latest
LACNIC=https://ftp.lacnic.net/pub/stats/lacnic/delegated-lacnic-latest

DLLIST="ARIN APNIC RIPENCC AFRINIC LACNIC"
CCLIST=""

iplistTime=$(date +%Y%m%d)
dbdate="$iplistTime"
configured=0

function cleancc() {
        rm -rf $execpath/$conffile/db/CC
}

function cleandl() {
        rm -rf $execpath/$conffile/dl
}

function download() {
        [ "$configured" == "0" ] && echo "not configured, downloading from all RA($DLLIST)"
        command -V wget > /dev/null && DLCLI="wget $DLCLIOPT -nv -O -" || DLCLI="curl $DLCLIOPT"
        [ -d $execpath/dl/$iplistTime ] || mkdir -p $execpath/dl/$iplistTime
        for NIC in $DLLIST; do
                $DLCLI ${!NIC} > $execpath/dl/$iplistTime/$NIC.txt
        done
}

function interactive() {
        configured=1
        echo "choose generate from newest db or exist db."
        select existdb in newest $(ls $execpath/dl); do
                if [ "$REPLY" == "1" ]; then
                        dbdate=$iplistTime
                else
                        dbdate=$existdb
                fi
                break
        done
}

function loadconfig() {
        [ -f $execpath/$conffile ] || return 1
        while read; do
                case $REPLY in
                	DL=)
                                read
                		DLLIST="$REPLY"
                   		;;
                       	GEN=)
                                read
                                CCLIST="$REPLY"
                    		;;
                        REPLACE)
                                cleancc
                                ;;
                esac
        done < $execpath/$conffile
        configured=1
}

while [ -n "$1" ]; do
	case $1 in
		-c)
			[ -z "$(command -v dirname)" ] && echo "dirname not installed or not in \$PATH." && exit
			[ -z "$(command -v basename)" ] && echo "basename not installed or not in \$PATH." && exit
			shift
			execpath="$(dirname $1)"
			conffile="$(basename $1)"
                        shift
			;;
                -4)
                        DLCLIOPT="-4"
                        ;;
                -6)
                        DLCLIOPT="-6"
                        ;;
                -d)
                        download
                        exit
                        ;;
                -i)
                        interactive
                        break
                        ;;
		*)
			echo "\
usage:	 iplist-gen.sh [options] [action]
options: -c     [config file location]
         -4
                using IPv4 for download
         -6
                using IPv6 for download
actions: -d
                download delegate db (defined in autogen.conf, defaults are ALL)
         -i
	        interactive mode"
			exit
			;;
	esac
done

[ "$configured" == "0" ] && loadconfig || echo "no config file at $execpath/$conffile or can't load, use -i instead."
[ -d $execpath/dl/$dbdate ] || download
[ -d $execpath/db/CC ] || mkdir -p $execpath/db/CC
for iplistCC in $CCLIST; do
	[ -f $execpath/db/CC/$iplistCC.txt ] && rm -rf $execpath/db/CC/$iplistCC.txt
done

function checkCC() {
	for CC in $CCLIST; do
		[ "$1" == "$CC" ] && return 0
	done
}
for NIC in $DLLIST; do
	while IFS='|' read Organization CountryCode Type Address Mask Date Stat; do
		checkCC $CountryCode $Type || continue
		case $Type in
			ipv4)
				declare -i CoverBit=0 Mask TrueMask
				while [ $Mask -gt 1 ]; do
					CoverBit=$CoverBit+1
					Mask=$Mask/2
				done
				TrueMask=32-$CoverBit
#                               Mask=32-$(bc -l <<< "l($Mask)/l(2)")
				echo $Address/$TrueMask >> $execpath/db/CC/$CountryCode.txt
				;;
			ipv6)
				echo $Address/$Mask >> $execpath/db/CC/$CountryCode.txt
				;;
		esac
	done < $execpath/dl/$dbdate/$NIC.txt
done
