#!/bin/bash
execpath=/etc/iplist

WGETCLI="wget -O -"
CURLCLI="curl"
PREFER=

SubFolder=CC

ARIN=https://ftp.arin.net/pub/stats/arin/delegated-arin-extended-latest	# only extended
APNIC=https://ftp.apnic.net/apnic/stats/apnic/delegated-apnic-latest
RIPENCC=https://ftp.ripe.net/pub/stats/ripencc/delegated-ripencc-latest
AFRINIC=https://ftp.afrinic.net/stats/afrinic/delegated-afrinic-latest
LACNIC=https://ftp.lacnic.net/pub/stats/lacnic/delegated-lacnic-latest

DLLIST="ARIN APNIC RIPENCC AFRINIC LACNIC"

# Format: Organization | CountryCode | Type | Address | Mask | Date | Stat
splitcountry() {
	while IFS='|' read Organization CountryCode Type Address Mask Date Stat; do
		if [[ $CountryCode =~ ^[A-Z]{2,2}$ ]]; then
			case $Type in
				ipv4)
					declare -i CoverBit=0 Mask TrueMask
					while [ $Mask -gt 1 ]; do
						CoverBit=$CoverBit+1
						Mask=$Mask/2
					done
					TrueMask=32-$CoverBit
					echo $Address/$TrueMask >> $execpath/inet/$SubFolder/$CountryCode.txt
				;;
				ipv6)
					echo $Address/$Mask >> $execpath/inet6/$SubFolder/$CountryCode.txt
				;;
			esac
		fi
	done
}

if [ ! -d $execpath/inet/$SubFolder ]; then
	mkdir $execpath/inet/$SubFolder
fi
if [ ! -d $execpath/inet6/$SubFolder ]; then
	mkdir $execpath/inet6/$SubFolder
fi

for NIC in $DLLIST; do
	wget -O - ${!NIC} | splitcountry
done
