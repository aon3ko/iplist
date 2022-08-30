#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <parameter.h>

#ifdef SELECT_IPSET
	#include <libipset/ipset.h>
#endif
#ifdef SELECT_NFT
	#include <nftables/libnftables.h>
	#include <realstring.h>
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
#ifdef SELECT_IPSET
	ipset_load_types();
	iplist->opIf = ipset_init();
#endif
#ifdef SELECT_NFT
	iplist->opIf = nft_ctx_new(NFT_CTX_DEFAULT);
	strcpy(iplist->nftTableName, "neko");
#endif
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
	char cmdBuf[256], readBuf[64], flag = 0;
	struct iplist_internal *objptr = NULL;
	unsigned int counter = 0;
#ifdef SELECT_NFT
	unsigned int counter4 = 0, counter6 = 0;
#endif

	// Check for existing
	for (int pos = 0; pos < iplist->size; pos++)
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
		sprintf(cmdBuf,"create %s-4 hash:net family inet", objptr->name);
		ipset_parse_line(iplist->opIf, cmdBuf);
		sprintf(cmdBuf,"create %s-6 hash:net family inet6", objptr->name);
		ipset_parse_line(iplist->opIf, cmdBuf);
#endif
#ifdef SELECT_NFT
		sprintf(cmdBuf, "add set ip %s %s { type ipv4_addr; flags interval; }", iplist->nftTableName, objptr->name);
		nft_run_cmd_from_buffer(iplist->opIf, cmdBuf);
		sprintf(cmdBuf, "add set ip6 %s %s { type ipv6_addr; flags interval; }", iplist->nftTableName, objptr->name);
		nft_run_cmd_from_buffer(iplist->opIf, cmdBuf);
#endif
	}

	// Every list should flush once
	if (objptr->flags != (V4_CLEARED | V6_CLEARED)) {
#ifdef SELECT_IPSET
		sprintf(cmdBuf, "flush %s-4", iplistName);
		ipset_parse_line(iplist->opIf, cmdBuf);
		sprintf(cmdBuf, "flush %s-6", iplistName);
		ipset_parse_line(iplist->opIf, cmdBuf);
#endif
#ifdef SELECT_NFT
		sprintf(cmdBuf, "flush set ip %s %s", iplist->nftTableName, objptr->name);
		nft_run_cmd_from_buffer(iplist->opIf, cmdBuf);
		sprintf(cmdBuf, "flush set ip6 %s %s", iplist->nftTableName, objptr->name);
		nft_run_cmd_from_buffer(iplist->opIf, cmdBuf);
#endif
		objptr->flags = V4_CLEARED | V6_CLEARED;
	}

	// Command compose and execution
#ifdef SELECT_NFT
	struct string *nftCmd4 = string_init(), *nftCmd6 = string_init();
	sprintf(cmdBuf, "add element ip %s %s { ", iplist->nftTableName, iplistName);
	string_append(nftCmd4, cmdBuf);
	sprintf(cmdBuf, "add element ip6 %s %s { ", iplist->nftTableName, iplistName);
	string_append(nftCmd6, cmdBuf);
#endif
	while (fgets(readBuf, 64, iplistDb) != NULL) {
#ifdef SELECT_IPSET
		if (strchr(readBuf, '.') != NULL)
			sprintf(cmdBuf, "add %s-4 %s", iplistName, readBuf);
		if (strchr(readBuf, ':') != NULL)
			sprintf(cmdBuf, "add %s-6 %s", iplistName, readBuf);
		ipset_parse_line(iplist->opIf, cmdBuf); counter++;
#endif
#ifdef SELECT_NFT
		if (strchr(readBuf, '.') != NULL) {
			sprintf(cmdBuf, " %s,", readBuf);
			string_append(nftCmd4, cmdBuf);
			counter4++;
		}
		if (strchr(readBuf, ':') != NULL) {
			sprintf(cmdBuf, " %s,", readBuf);
			string_append(nftCmd6, cmdBuf);
			counter6++;
		}
#endif
	}
#ifdef SELECT_NFT
	sprintf(cmdBuf, " }");
	string_append(nftCmd4, cmdBuf); string_append(nftCmd6, cmdBuf);
	if (counter4 > 0) nft_run_cmd_from_buffer(iplist->opIf, nftCmd4->str);
	if (counter6 > 0) nft_run_cmd_from_buffer(iplist->opIf, nftCmd6->str);
	string_delete(nftCmd4); string_delete(nftCmd6);
	counter = counter4 + counter6;
#endif

	fclose(iplistDb);
	printf(" done with %u record(s).\n", counter);
	return 0;
}

int main(int argc, char *argv[]) {
	struct iplist *iplist = iplist_init();

	PARAMPARSE(argc, argv)
        	switch (argv[pos][1]) {
		case 'c':
			break;
#ifdef SELECT_NFT
		case 't':
			strcpy(iplist->nftTableName, argv[pos + 1]);
			break;
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
			return EXIT_SUCCESS;
		}

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
