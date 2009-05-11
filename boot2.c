#include <stdlib.h>
#include <string.h>

#include "tools.h"
#include "nand.h"

#include "boot2.h"
#include "wii-fsck.h"

// this is so non-reentrant...
static u8 boot2[BYTES_PER_BLOCK * 3];
static u8 boot2_copy;
static u8 *boot2_tik = NULL;
static u8 *boot2_tmd = NULL;
static u8 *boot2_contents = NULL;

void boot2_load(rawnand nand, int copy)
{
	if (copy)
		copy = 1;
	boot2_copy = copy;

	memset(boot2, 0, BYTES_PER_BLOCK * 3);
	boot2_contents = boot2_tik = boot2_tmd = NULL;


	rawnand_read_block(nand, boot2, 1 + copy*4);
	rawnand_read_block(nand, boot2 + BYTES_PER_BLOCK, 2 + copy*4);
	rawnand_read_block(nand, boot2 + 2*BYTES_PER_BLOCK, 3 + copy*4);

	// FIXME: is this offset really static or does boot1 do some fancy calculation to get it?
	if (copy)
		memcpy(boot2, boot2 + 0x40000, (BYTES_PER_BLOCK*3) - 0x40000);
}

int boot2_check_signature()
{
	u32 header_len;
	u32 cert_len;
	u32 tik_len;
	u32 tmd_len;
	u32 offset;
	int result = WIIFSCK_OK;
	u8 *cert = NULL;

	header_len = be32(boot2);
	if (header_len > 0x80)
		return WIIFSCK_INVAL;

	cert_len = be32(boot2 + 8);
	tik_len = be32(boot2 + 0xc);
	tmd_len = be32(boot2 + 0x10);

	offset = header_len;

	cert = boot2 + offset;
	offset = offset + cert_len;

	boot2_tik = boot2 + offset;
	offset = offset + tik_len;

	boot2_tmd = boot2 + offset;
	boot2_contents = boot2 + round_up(offset + tmd_len, 0x40);

	if (check_cert_chain(boot2_tik, tik_len, cert, cert_len, 0) < 0)
	{
		fprintf(stderr, "boot2(%d): tik signature is invalid.\n",
				boot2_copy);
		if(check_cert_chain(boot2_tik, tik_len, cert, cert_len, 1) <
				0)
		{
			fprintf(stderr, "boot2(%d): tik signature is"
					"invalid even when using"
					"strncmp.\n", boot2_copy);
			return WIIFSCK_INVAL;
		}
		result = WIIFSCK_BUG;
	}
	if (check_cert_chain(boot2_tmd, tmd_len, cert, cert_len, 0) < 0)
	{
		fprintf(stderr, "boot2(%d): tmd signature is invalid.\n",
				boot2_copy);
		if (check_cert_chain(boot2_tmd, tmd_len, cert,
					cert_len, 1) < 0) {
			fprintf(stderr, "boot2(%d): tmd signature is invalid "
					"even when using strncmp.\n",
					boot2_copy);
			return WIIFSCK_INVAL;
		}
		result = WIIFSCK_BUG;
	}

	return result;
}

int boot2_check_contents()
{
	u32 n_contents;
	u32 i;
	u32 rounded_len;
	u8 *p;
	u8 *cnt;
	u8 *buffer;
	u8 fail;
	u8 iv[16];
	u8 title_key[16];
	u8 hash[20];

	if (boot2_tmd == NULL)
		boot2_check_signature();
	if (boot2_tmd == NULL)
		return WIIFSCK_INVAL;

	n_contents = be16(boot2_tmd + 0x01de);

	p = boot2_tmd + 0x01e4;
	cnt = boot2_contents;
	fail = 0;

	decrypt_title_key(boot2_tik, title_key);

	for (i = 0; i < n_contents; i++) {

		rounded_len = round_up(be64(p + 8), 0x40);

		buffer = malloc(rounded_len);
		if (buffer == NULL)
		{
			fprintf(stderr, "boot2(%d): unable to allocate %16llx bytes for content %d\n", boot2_copy, be64(p + 8), i);
			fail = 1;
			continue;
		}

		memset(iv, 0, 16);
		memcpy(iv, p + 4, 2);
		aes_cbc_dec(title_key, iv, cnt, rounded_len, buffer);

		sha(buffer, be64(p + 8), hash);
		free(buffer);

		if (memcmp(hash, p + 0x10, 0x14))
		{
			fprintf(stderr, "boot2(%d): hash verification failed for content %d\n", boot2_copy, i);
			hexdump(hash, 0x14);
			hexdump(p + 0x10, 0x14);
			fail = 1;
		}

		cnt = (u8 *)round_up((u64)cnt + be64(p + 8), 0x40);
		p += 0x24;
	}

	if (fail)
		return WIIFSCK_INVAL;
	else
		return WIIFSCK_OK;
}
