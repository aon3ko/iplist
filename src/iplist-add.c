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

struct iplist_internal {
	char name[64];
	char flags;
};

struct iplist {
	struct iplist_internal *objptr;
	void *opIf;
	unsigned int size;
#ifdef SELECT_NFT
	char nftTableName[64];
#endif
};

struct iplist *iplist_init() {
	struct iplist *iplist = calloc(1, sizeof(struct iplist));
	iplist->size = 0;
	iplist->objptr = calloc(0, sizeof(struct iplist_internal));
	return iplist;
}

void iplist_finish(struct iplist *iplist) {
	if (iplist->size > 0)
		free(iplist->objptr);
#ifdef SELECT_IPSET
	ipset_fini(iplist->opIf);
#endif
#ifdef SELECT_NFT
	nft_ctx_free(iplist->opIf);
#endif
	free(iplist);
}

int iplist_append(struct iplist *iplist, FILE *iplistDb, const char *iplistName) {
	char commandBuf[256], readBuf[64], flag = 0;
	int counter = 0;
	struct iplist_internal *objptr = NULL;

	// Check for existing
	for (int pos = 1; pos < iplist->size; pos++)
		if (strcmp((iplist->objptr + pos)->name, iplistName) == 0) {
			objptr = iplist->objptr + pos;
			break;
		}

	// If not existing, create new
	if (objptr == NULL) {
		iplist->objptr = realloc(iplist->objptr, sizeof(struct iplist_internal) * ++iplist->size);
		objptr = iplist->objptr + iplist->size - 1;
		strcpy(objptr->name, iplistName);
#ifdef SELECT_IPSET
		sprintf(commandBuf,"create %s-4 hash:net family inet", objptr->name);
		ipset_parse_line(iplist->opIf, commandBuf);
		sprintf(commandBuf,"create %s-6 hash:net family inet6", objptr->name);
		ipset_parse_line(iplist->opIf, commandBuf);
#endif
#ifdef SELECT_NFT
		sprintf(commandBuf, "add set ip %s %s { type ipv4_addr; flags interval; }", iplist->nftTableName, objptr->name);
		nft_run_cmd_from_buffer(iplist->opIf, commandBuf);
		sprintf(commandBuf, "add set ip6 %s %s { type ipv6_addr; flags interval; }", iplist->nftTableName, objptr->name);
		nft_run_cmd_from_buffer(iplist->opIf, commandBuf);
#endif
	}

	// Every list should flush once
	if (objptr->flags != (V4_CLEARED | V6_CLEARED)) {
#ifdef SELECT_IPSET
		sprintf(commandBuf, "flush %s-4", iplistName);
		ipset_parse_line(iplist->opIf, commandBuf);
		sprintf(commandBuf, "flush %s-6", iplistName);
		ipset_parse_line(iplist->opIf, commandBuf);
#endif
#ifdef SELECT_NFT
		sprintf(commandBuf, "flush set ip %s %s", iplist->nftTableName, objptr->name);
		nft_run_cmd_from_buffer(iplist->opIf, commandBuf);
		sprintf(commandBuf, "flush set ip6 %s %s", iplist->nftTableName, objptr->name);
		nft_run_cmd_from_buffer(iplist->opIf, commandBuf);
#endif
		objptr->flags = V4_CLEARED | V6_CLEARED;
	}

	// Command compose and execution
#ifdef SELECT_NFT
	struct string nftcommand4 = string_init(), nftcommand6 = string_init();
	sprintf(commandBuf, "add element ip %s %s { ", iplist->nftTableName, iplistName);
	string_append(&nftcommand4, commandBuf);
	sprintf(commandBuf, "add element ip6 %s %s { ", iplist->nftTableName, iplistName);
	string_append(&nftcommand6, commandBuf);
#endif
	while (fgets(readBuf, 64, iplistDb) != NULL) {
#ifdef SELECT_IPSET
		if (strchr(readBuf, '.') != NULL)
			sprintf(commandBuf, "add %s-4 %s", iplistName, readBuf);
		if (strchr(readBuf, ':') != NULL)
			sprintf(commandBuf, "add %s-6 %s", iplistName, readBuf);
		ipset_parse_line(iplist->opIf, commandBuf); counter++;
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
	nft_run_cmd_from_buffer(iplist->opIf, nftcommand4.str);
	nft_run_cmd_from_buffer(iplist->opIf, nftcommand6.str);
	string_delete(&nftcommand4); string_delete(&nftcommand6);
#endif

	fclose(iplistDb);
	printf(" done with %d record(s).\n", counter);
	return 0;
}

int main(int argc, char *argv[]) {
	struct iplist *iplist = iplist_init();;

	for (int pos = 1; pos < argc; pos++)
        	if (argv[pos][0] == '-') switch (argv[pos][1]) {
		case 'c':
			break;
#ifdef SELECT_NFT
		case 't':
			strcpy(iplist->nftTableName, argv[pos + 1]);
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

#ifdef SELECT_IPSET
	ipset_load_types();
	iplist->opIf = ipset_init();
#endif
#ifdef SELECT_NFT
	iplist->opIf = nft_ctx_new(NFT_CTX_DEFAULT);
	strcpy(iplist->nftTableName, "neko");
#endif

	char	iplistFile[64],
		iplistName[64],
		iplistDbLocation[256],
		letyoureadinwithhappy[256];

	char confFileLocation[256];
	sprintf(confFileLocation, "%s/%s", workpath, conffile);
	FILE *autoadd = fopen(confFileLocation, "r");

	if (autoadd != NULL) {
		while (fscanf(autoadd, "%s %s %s", iplistFile, iplistName, letyoureadinwithhappy) != EOF) {
			sprintf(iplistDbLocation, "%s/db/%s", workpath, iplistFile);
			printf("adding %s to %s...", iplistFile, iplistName);
			iplist_append(iplist, fopen(iplistDbLocation, "r"), iplistName);
		}
		fclose(autoadd);
	} else printf("config file? where?\n");

	iplist_finish(iplist);
	return EXIT_SUCCESS;
}
