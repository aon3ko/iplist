#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libneko/realstring.c>

#define SELECT_NFT
#ifdef SELECT_IPSET
	#include <libipset/ipset.h>
	#define SELECTED
#endif
#ifdef SELECT_NFT
	#include <nftables/libnftables.h>
	#define SELECTED
#endif
#ifndef SELECTED
	#include <libipset/ipset.h>
	#include <nftables/libnftables.h>
#endif

char workpath[128] = "/etc/iplist";
char conffile[64] = "autoadd.conf";

enum iplist_flags {
	V4_CLEARED = (1 << 0),
	V6_CLEARED = (1 << 1),
};

union iplist {
	struct {
		char name[64];
		char flags;
	};
	struct {
		unsigned int size;
		void *operationInterface;
	};
};

union iplist *iplist_init(void *operationInterface) {
	char commandBuf[256];
	union iplist *iplist = calloc(1, sizeof(union iplist));
	iplist->size = 1;
	iplist->operationInterface = operationInterface;
/*
#ifdef SELECT_IPSET
	sprintf(commandBuf,"-n list");
	ipset_parse_line(iplist->operationInterface, commandBuf);
#endif
#ifdef SELECT_NFT
	sprintf(commandBuf,"list sets");
	nft_run_cmd_from_buffer(iplist->operationInterface, commandBuf);
	char nftLineBuf[256];
	while (fgets(nftLineBuf, 256, nft_ctx_get_output_buffer(iplist->operationInterface))) {

	}
#endif
*/
	return iplist;
}

int iplist_append(union iplist **dst, FILE *iplistDb, const char *iplistName, const char *p) {
	char commandBuf[256], readBuf[64], flag = 0;
	int counter = 0;
	union iplist *currentLocation = NULL;
#ifdef SELECT_NFT
	struct string nftcommand4 = string_init(), nftcommand6 = string_init();
	sprintf(commandBuf, "add element ip %s %s { ", p, iplistName);
	string_append(&nftcommand4, commandBuf);
	sprintf(commandBuf, "add element ip6 %s %s { ", p, iplistName);
	string_append(&nftcommand6, commandBuf);
#endif

	for (int pos = 1; pos < (*dst)->size; pos++)
		if (strcmp((*dst)->name, iplistName) == 0) {
			currentLocation = (*dst) + pos;
			break;
		}
	if (currentLocation == NULL) {
		*dst = realloc(*dst, sizeof(union iplist) * (++(*dst)->size));
		currentLocation = (*dst) + (*dst)->size - 1;
		strcpy(currentLocation->name, iplistName);
#ifdef SELECT_IPSET
		sprintf(commandBuf,"create %s-4 hash:net family inet", currentLocation->name);
		ipset_parse_line((*dst)->operationInterface, commandBuf);
		sprintf(commandBuf,"create %s-6 hash:net family inet6", currentLocation->name);
		ipset_parse_line((*dst)->operationInterface, commandBuf);
#endif
#ifdef SELECT_NFT
		sprintf(commandBuf, "add set ip %s %s { type ipv4_addr; flags interval; }", p, currentLocation->name);
		nft_run_cmd_from_buffer((*dst)->operationInterface, commandBuf);
		sprintf(commandBuf, "add set ip6 %s %s { type ipv6_addr; flags interval; }", p, currentLocation->name);
		nft_run_cmd_from_buffer((*dst)->operationInterface, commandBuf);
#endif
		currentLocation->flags = V4_CLEARED | V6_CLEARED;
	}

	if (currentLocation->flags != (V4_CLEARED | V6_CLEARED)) {
#ifdef SELECT_IPSET
#endif
#ifdef SELECT_NFT
#endif
	}

	while (fgets(readBuf, 64, iplistDb) != NULL) {
#ifdef SELECT_IPSET
		if (strchr(readBuf, '.') != NULL)
			sprintf(commandBuf, "add %s-4 %s", iplistName, readBuf);
		if (strchr(readBuf, ':') != NULL)
			sprintf(commandBuf, "add %s-6 %s", iplistName, readBuf);
		ipset_parse_line((*dst)->operationInterface, commandBuf); counter++;
#endif
#ifdef SELECT_NFT
		if (strchr(readBuf, '.') != NULL) {
			sprintf(commandBuf, " %s,", readBuf);
			string_append(&nftcommand4, commandBuf);
			counter++;
		}
		if (strchr(readBuf, ':') != NULL) {
			sprintf(commandBuf, " %s,", readBuf);
			string_append(&nftcommand6, commandBuf);
			counter++;
		}
#endif
	}
#ifdef SELECT_NFT
	sprintf(commandBuf, " }", readBuf);
	string_append(&nftcommand4, commandBuf); string_append(&nftcommand6, commandBuf);
	nft_run_cmd_from_buffer((*dst)->operationInterface, nftcommand4.str);
	nft_run_cmd_from_buffer((*dst)->operationInterface, nftcommand6.str);
	string_delete(&nftcommand4); string_delete(&nftcommand6);
#endif

	fclose(iplistDb);
	printf(" done with %d record(s).\n", counter);
	return 0;
}

void wa_ipset_print() {

}

int main(int argc, char *argv[]) {
#ifdef SELECT_IPSET
	ipset_load_types();
	struct ipset *operationInterface = ipset_init();
	ipset_custom_printf(operationInterface, NULL, NULL, ipsetPrint, NULL);
#endif
#ifdef SELECT_NFT
	struct nft_ctx *operationInterface = nft_ctx_new(NFT_CTX_DEFAULT);
	//nft_ctx_buffer_output(operationInterface);
	char nft_table_name[64] = "neko";
#endif
	union iplist *iplist = iplist_init(operationInterface);

	for (int pos = 1; pos < argc; pos++)
        	if (argv[pos][0] == '-') switch (argv[pos][1]) {
		case 'c':
			break;
#ifdef SELECT_NFT
		case 't':
			strcpy(nft_table_name, argv[pos + 1]);
			break;
#endif
#ifndef SELECTED
		case '-':
#endif
		default:
			printf("\
usage:	 iplist-add [options]\n\
options: -c <config file location>\n\
	    default location at /etc/iplist/autoadd.conf\n");
#ifdef SELECT_IPSET
			printf("Compiled with ipset support\n");
#endif
#ifdef SELECT_NFT
			printf("\
	 -t <table name>\n\
	    add to which table.\n\
Compiled with nft support\n");
#endif
#ifndef SELECTED
			printf("\
	 --ipset\n\
	    use ipset.\n\
	 --nft\n\
	    use nft.\n\
	 -t <table name>\n\
	    add to which table.\n");
#endif
			return EXIT_SUCCESS;
		}

	char	iplistFile[64],
		iplistName[64],
		iplistDbLocation[256],
		letyoureadinwithhappy[256];

	char confFileLocation[256];
	sprintf(confFileLocation, "%s/%s", workpath, conffile);
	FILE *autoadd = fopen(confFileLocation, "r");

	if (autoadd != NULL) while (fscanf(autoadd, "%s %s %s", iplistFile, iplistName, letyoureadinwithhappy) != EOF) {
		sprintf(iplistDbLocation, "%s/db/%s", workpath, iplistFile);
		printf("adding %s to %s...", iplistFile, iplistName);
		iplist_append(&iplist, fopen(iplistDbLocation, "r"), iplistName, nft_table_name);
	} else printf("config file? where?\n");
	fclose(autoadd);

#ifdef SELECT_IPSET
	ipset_fini(operationInterface);
#endif
#ifdef SELECT_NFT
	nft_ctx_free(operationInterface);
#endif
	return EXIT_SUCCESS;
}
