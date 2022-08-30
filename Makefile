all: iplist-add

iplist-add: src/iplist-add.c libneko/realstring.o
	gcc src/iplist-add.c libneko/realstring.o -D SELECT_NFT -o iplist-add -I ./libneko/include -l nftables

libneko/realstring.o:
	$(MAKE) -C ./libneko

clean:
	-rm iplist-add
