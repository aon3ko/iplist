#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libipset/ipset.h>

#define LENGTH_OF_TYPE 16
#define LENGTH_OF_NAME 64
#define LENGTH_OF_PATH 128
#define LENGTH_OF_LOCATION 256

char workpath[LENGTH_OF_PATH] = "/etc/iplist";
char conffile[LENGTH_OF_NAME] = "autoadd.conf";

int main(int argc, char *argv[]) {
	ipset_load_types();
	struct ipset *ipsetInterface = ipset_init();

	for (int pos = 1; pos < argc; pos++) {
		if (strcmp(argv[pos], "-c") == 0)
			;
		else {
			printf("\
usage:	 iplist-addipset.sh [options]\n\
options: -c [config file location]\n\
	 default location at /etc/iplist/autoadd.conf\n");
			return 0;
		}
	}

	char	*confFileLocation = malloc(LENGTH_OF_LOCATION);
	sprintf(confFileLocation, "%s/%s", workpath, conffile);
	FILE	*autoadd = fopen(confFileLocation, "r"),
		*iplistDb = fopen(confFileLocation, "r");
	char	*iplistFile = malloc(LENGTH_OF_NAME),
		*iplistName = malloc(LENGTH_OF_NAME),
		*iplistType = malloc(LENGTH_OF_TYPE),
		*iplistDbLocation = malloc(LENGTH_OF_LOCATION),
		*unProcessedString = malloc(LENGTH_OF_NAME),
		*ipsetCommandBuf = malloc(LENGTH_OF_LOCATION),
		mode;
	int	counter;

	char *flushedSets[LENGTH_OF_LOCATION];
	for (int pos = 0; pos < LENGTH_OF_LOCATION; pos++)
		flushedSets[pos] = calloc(LENGTH_OF_NAME, sizeof(char));
	char *ipsetArgs[8];
	for (int pos = 0; pos < 8; pos++)
		ipsetArgs[pos] = malloc(LENGTH_OF_NAME);

	while (fscanf(autoadd, "%s %s %s", iplistFile, iplistName, iplistType) != EOF) {
		sprintf(iplistDbLocation, "%s/db/%s", workpath, iplistFile);
		freopen(iplistDbLocation, "r", iplistDb);
		for (int pos = 0; pos < LENGTH_OF_LOCATION; pos++) {
			if (strcmp(flushedSets[pos], iplistName) == 0)
				break;
			else if (flushedSets[pos][0] == 0) {
				strcpy(flushedSets[pos], iplistName);
				for (int pos2 = 0; pos2 < 8; pos2++)
					memset(ipsetArgs[pos2], 0, LENGTH_OF_NAME);
				strcpy(ipsetArgs[1], "-!");
				strcpy(ipsetArgs[2], "create");
				sprintf(ipsetArgs[3], "%s-4", iplistName);
				sprintf(ipsetArgs[4], "hash:%s", iplistType);
				strcpy(ipsetArgs[5], "family");
				strcpy(ipsetArgs[6], "inet");
				ipset_parse_argv(ipsetInterface, 7, ipsetArgs);
				sprintf(ipsetArgs[3], "%s-6", iplistName);
				strcpy(ipsetArgs[6], "inet6");
				ipset_parse_argv(ipsetInterface, 7, ipsetArgs);
				strcpy(ipsetArgs[2], "flush");
				sprintf(ipsetArgs[3], "%s-4", iplistName);
				ipset_parse_argv(ipsetInterface, 4, ipsetArgs);
				sprintf(ipsetArgs[3], "%s-6", iplistName);
				ipset_parse_argv(ipsetInterface, 4, ipsetArgs);
				break;
			}
		}
		printf("adding %s ... ", iplistFile);
		counter = 0;
		while (fgets(unProcessedString, LENGTH_OF_NAME, iplistDb) != NULL) {
			mode = 0;
			for (int pos = 0; pos < LENGTH_OF_NAME; pos++) {
				switch (unProcessedString[pos]) {
				case '.':
					mode |= 0x01;
					break;
				case ':':
					mode |= 0x02;
					break;
				case 10:
				case 13:
					unProcessedString[pos] = 0;
					break;
				}
			}
			switch (mode) {
			case 0x01:
				sprintf(ipsetCommandBuf, "add %s-4 %s", iplistName, unProcessedString);
				ipset_parse_line(ipsetInterface, ipsetCommandBuf); counter++;
				break;
			case 0x02:
				sprintf(ipsetCommandBuf, "add %s-6 %s", iplistName, unProcessedString);
				ipset_parse_line(ipsetInterface, ipsetCommandBuf); counter++;
				break;
			default:
				if (strcmp(unProcessedString, ""))
					printf("%s cannot be add", unProcessedString);
			}
			memset(unProcessedString, 0, LENGTH_OF_NAME);
		}
		printf("done, %d record(s) added.\n", counter);
	}

	fclose(iplistDb);
	fclose(autoadd);
	ipset_fini(ipsetInterface);

	return 0;
}
