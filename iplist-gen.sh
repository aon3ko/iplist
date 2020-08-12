#!/bin/bash

execpath=/etc/iplist
conffile=genlist.conf

WGETCLI="wget -O -"
CURLCLI="curl"
PREFER=WGETCLI

SubFolder=CC

ARIN=https://ftp.arin.net/pub/stats/arin/delegated-arin-extended-latest	# only extended
APNIC=https://ftp.apnic.net/apnic/stats/apnic/delegated-apnic-latest
RIPENCC=https://ftp.ripe.net/pub/stats/ripencc/delegated-ripencc-latest
AFRINIC=https://ftp.afrinic.net/stats/afrinic/delegated-afrinic-latest
LACNIC=https://ftp.lacnic.net/pub/stats/lacnic/delegated-lacnic-latest

DLLIST="ARIN APNIC RIPENCC AFRINIC LACNIC"
ProtoLIST="inet inet6"
Mode="Minus"
CCLIST=""

checkCC() {
	_tmp=$(cut -b 1,2 <<< $1)
	for CC in $CCLIST; do
		case $Mode in
			Plus)
				[ $_tmp == $CC ] && return 0
				;;
			Minus)
				[ $_tmp == $CC ] && return 1
				;;
		esac
	done
	if [ $Mode == "Minus" ]; then
		return 0
	else
		return 1
	fi
}

# Format: Organization | CountryCode | Type | Address | Mask | Date | Stat
splitCountry() {
	while IFS='|' read Organization CountryCode Type Address Mask Date Stat; do
		if [[ $CountryCode =~ ^[A-Z]{2,2}$ ]]; then
		if CheckCC $CountryCode; then
			case $Type in
				ipv4)
					declare -i CoverBit=0 Mask TrueMask
					while [ $Mask -gt 1 ]; do
						CoverBit=$CoverBit+1
						Mask=$Mask/2
					done
					TrueMask=32-$CoverBit
					echo $Address/$TrueMask >> $execpath/db/$SubFolder/$CountryCode.txt
					;;
				ipv6)
					echo $Address/$Mask >> $execpath/db/$SubFolder/$CountryCode.txt
					;;
			esac
		fi
		fi
	done
}

for Protocol in $ProtoLIST; do
	[ ! -d $execpath/$Protocol/$SubFolder ] && mkdir -p $execpath/$Protocol/$SubFolder && continue
	while read FileName; do
		if checkCC $FileName; then
			rm -f $execpath/$Protocol/$SubFolder/$FileName
		fi
	done <<< $(ls $execpath/$Protocol/$SubFolder)
done

for NIC in $DLLIST; do
	wget -O - ${!NIC} | splitCountry
done
