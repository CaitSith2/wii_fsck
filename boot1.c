#include <stdlib.h>
#include <string.h>

#include "tools.h"
#include "nand.h"
#include "wii-fsck.h"

static u8 boot1_hash[0x14] = "\xb5\xbd\x9d\x49\x62\x69\xda\x89\x0b\xc2\xd8\x3d\xfc\x0f\x91\x20\x8f\x61\xa6\x12";

int boot1_check(rawnand nand)
{
	u8 *buffer;
	u8 hash[20];
	
	buffer = malloc(BYTES_PER_BLOCK);
	if(!buffer)
		fatal("Unable to allocate boot1 buffer");
	
	rawnand_read_block(nand, buffer, 0);
	sha(buffer, 0x17800, hash);
	free(buffer);

	if (memcmp(hash, boot1_hash, 0x14)) {
		fprintf(stderr, "boot1: hash verification failed.\n");
		return WIIFSCK_INVAL;
	}

	return WIIFSCK_OK;
}
