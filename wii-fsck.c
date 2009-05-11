#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tools.h"
#include "my_getopt.h"
#include "nand.h"
#include "wii-fsck.h"
#include "nandfs.h"
#include "boot1.h"
#include "boot2.h"
#include "titles.h"
#include "uid.h"
#include "iplsave.h"

static rawnand nand;

static int boot1 = -1;

static int boot2_sig = -1;
static int boot2_cnt = -1;
static int boot2c_sig = -1;
static int boot2c_cnt = -1;

static int title_12 = -1;
static int title_12_ios = -1;

static int chans_valid = -1;

static void print_result(const char *info, int result)
{
	printf("  %20s    ", info);

	switch (result) {
	case WIIFSCK_OK:
		printf("OK\n");
		break;
	case WIIFSCK_INVAL:
		printf("Invalid\n");
		break;
	case WIIFSCK_BUG:
		printf("BUG\n");
		break;
	default:
		printf("%d\n", result);
		break;
	}
}

static void print_info(const char *a, const char *b)
{
	printf("  %20s    %s\n", a, b);
}

static void print_title_result(const char *name, int tid_hi, int tid_low, int result)
{
	printf("%s (%08x-%08x):\n", name, tid_hi, tid_low);

	print_info("TMD", (result & TITLE_TMDENOENT) ? "Not found" : "OK");
	print_info("Ticket", (result & TITLE_TIKENOENT) ? "Not found" : "OK");
	print_info("UID Entry", (result & TITLE_UIDENOENT) ? "Not found" : "OK");

	if(result & TITLE_SIGSTRNCMP)
		print_info("signature", "BUG");
	else if(result & TITLE_SIGINVAL)
		print_info("signature", "Invalid");
	else
		print_info("signature", "OK");

	print_info("contents", (result & TITLE_CONTENTENOENT) ?
			">= 1 not found!": "OK");
	print_info("content hashes", (result & TITLE_CONTENTHASH) ?
			"Invalid" : "OK");
}

static void print_help() {
	printf("usage: wii-fsck [options] nandfilename\n");
	printf("Valid options:\n");
	printf("  -n, --name=NAME    Load wii-specific keys from ~/.wii/NAME\n");
	printf("  -o OTP             Load keys from the given OTP dump instead of using ~/.wii/\n");
	printf("  -v, --verbose      Increase verbosity, can be specified multiple times\n");
	printf("\n");
	printf("  -h, --help         Display this help and exit\n");
	printf("  -V, --version      Print version and exit\n");
}

int main(int argc, char *argv[])
{
	char *otp = NULL;
  char nandotp = 0;
	printf("wii-fsck\n");

	char wiiname[256] = {0};

	static const struct option wiifsck_options[] = {
		{ "help", no_argument, 0, 'h' },
		{ "version", no_argument, 0, 'V' },
		{ "name", required_argument, 0, 'n' },
		{ "otp", required_argument, 0, 'o' },
		{ "verbose", no_argument, 0, 'v' },
    { "nandotp", no_argument, 0, 'N' },
		{ 0, 0, 0, 0 }
	};
	int c = 0;
	int optionindex;
  
	while(c >= 0) {
		c = my_getopt_long(argc, argv, "-hvno:", wiifsck_options, &optionindex);
		switch (c) {
			case 'n':
				strncpy(wiiname, my_optarg, 255);
				break;
      case 'N':
        nandotp = 1;
        break;
			case 'v':
				verbosity_level++;
				break;
			case 'h':
				print_help();
				exit(0);
			case 'V':
				printf(WIIFSCK_VERSION_STRING);
				printf("\n");
				exit(0);
			case 'o':
				otp = strdup(my_optarg);
				break;
			case '?':
				printf("Invalid option -%c. Try -h\n", my_optopt);
				exit(-1);
			case 1:
				nand = rawnand_open(my_optarg);
				if (nand == NULL)
					fatal("Unable to open nand %s", argv[1]);
				break;
		}
	}
	
	if (nand == NULL) {
		printf("You must specify a NAND dump file. See -h\n");
		exit(-1);
	}

	if ((otp == NULL) && (nandotp == 0)) {
		if (wiiname[0] == 0) {
			strcpy(wiiname, "default");
		}
		printf("loading keys for wii %s...\n", wiiname);
		load_keys(wiiname);
	} else if (otp != NULL) {
		printf("loading keys from OTP %s...\n", otp);
		load_keys_otp(otp);
		 free(otp);
	} else {
    printf("loading keys from BackupMii nand dump...\n");
    load_keys_nand_otp(nand);
  } 

	printf("doing stuff, please wait...\n");

	printf("checking boot1...\n");
	boot1 = boot1_check(nand);

	printf("checking boot2...\n");
	boot2_load(nand, 0);
	boot2_sig = boot2_check_signature();
	boot2_cnt = boot2_check_contents();

	printf("checking second copy of boot2...\n");
	boot2_load(nand, 1);
	boot2c_sig = boot2_check_signature();
	boot2c_cnt = boot2_check_contents();
	
	printf("checking filesystem...\n");
	struct nandfs *fs = nandfs_mount(nand);
	titles_init(fs);
	uid_init(fs);

	printf("checking 1-2...\n");
	title_12 =  check_title_integrity(1, 2);


	printf("checking 1-2 IOS...\n");
	int title_12_tmd_size, title_12_ios_version;
	struct signed_tmd *title_12_tmd;

	get_signed_tmd_size(1, 2, &title_12_tmd_size);
	title_12_tmd = malloc(title_12_tmd_size);

	if(title_12_tmd == NULL)
		fatal("Could not allocate memory for 1-2 tmd");

	get_signed_tmd(1, 2, title_12_tmd, NULL);
	title_12_ios_version = title_12_tmd->data.sys_version & 0xffffffff;
	title_12_ios = check_title_integrity(1, title_12_ios_version);
	free(title_12_tmd);

	printf("checking channel validity...\n");
	iplsave_init(fs);
	chans_valid = check_channel_validity();

	printf("\nwii-fsck report:\n");
	print_result("boot1", boot1);
	print_result("boot2 signature", boot2_sig);
	print_result("boot2 contents", boot2_cnt);
	print_result("2nd boot2 signature", boot2c_sig);
	print_result("2nd boot2 contents", boot2c_cnt);

	printf("\n");
	print_title_result("System Menu", 1, 2, title_12);
	print_title_result("System Menu IOS", 1, title_12_ios_version, title_12_ios);
	printf("\n");
	print_result("Channel validity", chans_valid);
	
	return 0;
}
