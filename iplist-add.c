#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libipset/ipset.h>

#define LENGTH_OF_TYPE 15
#define LENGTH_OF_NAME 63
#define LENGTH_OF_PATH 127
#define LENGTH_OF_LOCATION 255

char workpath[LENGTH_OF_PATH] = "/etc/iplist";
char conffile[LENGTH_OF_NAME] = "autoadd.conf";

// char *argvWrapper(char argv[]);

int main(int argc, char *argv[]) {
	ipset_load_types();
	struct ipset *ipsetInterface = ipset_init();

	for (int pos = 1; pos < argc; pos++)
        	if (argv[pos][0] == '-') switch (argv[pos][1]) {
		case 'c':
			break;
		default:
			printf("\
usage:	 iplist-add [options]\n\
options: -c <config file location>\n\
	    default location at /etc/iplist/autoadd.conf\n");
			return EXIT_SUCCESS;
		}

	char	iplistFile[LENGTH_OF_NAME],
		iplistName[LENGTH_OF_NAME],
		iplistType[LENGTH_OF_TYPE],
		iplistDbLocation[LENGTH_OF_LOCATION],
		flushedSets[LENGTH_OF_LOCATION][LENGTH_OF_NAME],
		*argvWrapper[LENGTH_OF_TYPE],
		ipsetArgs[LENGTH_OF_TYPE][LENGTH_OF_NAME],
		readBuffer[LENGTH_OF_NAME],
		ipsetCommandBuf[LENGTH_OF_LOCATION];
	memset(flushedSets, 0, LENGTH_OF_LOCATION * LENGTH_OF_NAME);
	for (int pos = 0; pos < LENGTH_OF_TYPE; pos++)
		argvWrapper[pos] = ipsetArgs[pos];

	char confFileLocation[LENGTH_OF_LOCATION];
	sprintf(confFileLocation, "%s/%s", workpath, conffile);
	FILE	*autoadd = fopen(confFileLocation, "r"),
		*iplistDb = fopen(confFileLocation, "r");
	if (autoadd != NULL) while (fscanf(autoadd, "%s %s %s", iplistFile, iplistName, iplistType) != EOF) {
		sprintf(iplistDbLocation, "%s/db/%s", workpath, iplistFile);
		freopen(iplistDbLocation, "r", iplistDb);
		for (int pos = 0; pos < LENGTH_OF_LOCATION; pos++)
			if (strcmp(flushedSets[pos], iplistName) == 0)
				break;
			else if (flushedSets[pos][0] == 0) {
				strcpy(flushedSets[pos], iplistName);
				memset(ipsetArgs, 0, LENGTH_OF_TYPE * LENGTH_OF_NAME);
				strcpy(ipsetArgs[1], "-!");
				strcpy(ipsetArgs[2], "create");
				sprintf(ipsetArgs[3], "%s-4", iplistName);
				sprintf(ipsetArgs[4], "hash:%s", iplistType);
				strcpy(ipsetArgs[5], "family");
				strcpy(ipsetArgs[6], "inet");
				ipset_parse_argv(ipsetInterface, 7, argvWrapper);
				sprintf(ipsetArgs[3], "%s-6", iplistName);
				strcpy(ipsetArgs[6], "inet6");
				ipset_parse_argv(ipsetInterface, 7, argvWrapper);
				strcpy(ipsetArgs[2], "flush");
				sprintf(ipsetArgs[3], "%s-4", iplistName);
				ipset_parse_argv(ipsetInterface, 4, argvWrapper);
				sprintf(ipsetArgs[3], "%s-6", iplistName);
				ipset_parse_argv(ipsetInterface, 4, argvWrapper);
				break;
			}

		printf("adding %s ... ", iplistFile);
		int counter = 0;
		while (fgets(readBuffer, LENGTH_OF_NAME, iplistDb) != NULL) {
			char mode = 0;
			for (int pos = 0; pos < LENGTH_OF_NAME; pos++)
				switch (readBuffer[pos]) {
				case '.':
					mode |= 0x01;
					break;
				case ':':
					mode |= 0x02;
					break;
				case 10:
				case 13:
					readBuffer[pos] = 0;
					break;
				}
			switch (mode) {
			case 0x01:
				sprintf(ipsetCommandBuf, "add %s-4 %s", iplistName, readBuffer);
				ipset_parse_line(ipsetInterface, ipsetCommandBuf); counter++;
				break;
			case 0x02:
				sprintf(ipsetCommandBuf, "add %s-6 %s", iplistName, readBuffer);
				ipset_parse_line(ipsetInterface, ipsetCommandBuf); counter++;
				break;
			default:
				if (strcmp(readBuffer, ""))
					printf("\n%s cannot be add", readBuffer);
			}
			memset(readBuffer, 0, LENGTH_OF_NAME);
		}
		printf("done, %d record(s) added.\n", counter);
	} else printf("config file? where?\n");
	fclose(iplistDb);
	fclose(autoadd);

	ipset_fini(ipsetInterface);
	return EXIT_SUCCESS;
}

/*
char *argvWrapper(char argv[]) {
}
*/
