#include <stdlib.h>
#include <string.h>
#include "tools.h"
#include "nandfs.h"
#include "shared1.h"
#include "iplsave.h"
#include "titles.h"
#include "wii-fsck.h"

// This chunk will check the validity of iplsave.bin,
// as well as read it.

#define IPLSAVE_DEBUG
#ifdef IPLSAVE_DEBUG
#	define	dbgprintf(f, arg...) do { printf("iplsave: " f, ##arg); } while(0)
#else
#	define	dbgprintf(f, arg...) 
#endif

struct nandfs *__iplsave_fs = NULL;
iplsave *__iplsave_list = NULL;
int __iplsave_size = 0;
int __iplsave_inited = 0;

static int __check_iplsave_sanity()
{
	if((__iplsave_list->magic[0] != 'R') || (__iplsave_list->magic[1] != 'I') || (__iplsave_list->magic[2] != 'P') || (__iplsave_list->magic[3] != 'L')) {
		dbgprintf("iplsave.bin header is not \'RIPL\' (%02x%02x%02x%02x). It is \'%c%c%c%c\' (%02x%02x%02x%02x)\n", 'R', 'I', 'P', 'L', __iplsave_list->magic[0], __iplsave_list->magic[1], __iplsave_list->magic[2], __iplsave_list->magic[3], __iplsave_list->magic[0], __iplsave_list->magic[1], __iplsave_list->magic[2], __iplsave_list->magic[3]);
		return 0;
	}
	if(be32((u8 *)&__iplsave_list->filesize) != __iplsave_size) {
		dbgprintf("iplsave.bin (%lu) is not the same size as the header said it should be (%lu)\n", (u32)__iplsave_size, __iplsave_list->filesize);
		return 0;
	}
	u8 md5hash[0x10];
	md5((u8*)__iplsave_list, __iplsave_size - 0x10, md5hash);
	if(memcmp(md5hash, __iplsave_list->md5_sum, 0x10) != 0) {
		dbgprintf("iplsave.bin's hash (%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x) is invalid! Should be (%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x)!\n", __iplsave_list->md5_sum[0], __iplsave_list->md5_sum[1], __iplsave_list->md5_sum[2], __iplsave_list->md5_sum[3], __iplsave_list->md5_sum[4], __iplsave_list->md5_sum[5], __iplsave_list->md5_sum[6], __iplsave_list->md5_sum[7], __iplsave_list->md5_sum[8], __iplsave_list->md5_sum[9], __iplsave_list->md5_sum[0xA], __iplsave_list->md5_sum[0xB], __iplsave_list->md5_sum[0xC], __iplsave_list->md5_sum[0xD], __iplsave_list->md5_sum[0xE], __iplsave_list->md5_sum[0xF], md5hash[0], md5hash[1], md5hash[2], md5hash[3], md5hash[4], md5hash[5], md5hash[6], md5hash[7], md5hash[8], md5hash[9], md5hash[0xA], md5hash[0xB], md5hash[0xC], md5hash[0xD], md5hash[0xE], md5hash[0xF]);
		return 0;
	}
	int i;
	int retval = 1;
	for(i = 0; i < 0x30; i++) {
		if((__iplsave_list->channels[i].channel_type != 0) && (__iplsave_list->channels[i].channel_type != 1)) {
			u64 tidtmp = be64((u8 *)&__iplsave_list->channels[i].title_id);
			int result = check_title_integrity(TITLE_HIGH(tidtmp), TITLE_LOW(tidtmp));
			int error = 0;
			if(result & TITLE_TMDENOENT)		error = 1;
			if(result & TITLE_TIKENOENT)		error = 1;
			if(result & TITLE_UIDENOENT)		error = 1;
			if(result & TITLE_SIGINVAL)		error = 1;
			if(result & TITLE_CONTENTENOENT)	error = 1;
			if(result & TITLE_CONTENTHASH)		error = 1;
      if(TITLE_LOW(tidtmp)==0x48415858) {
        error = 0;
        dbgprintf("Homebrew channel Installed, skipping its validity check, since it is fakesigned.\n");
      }
			if(error) {
				dbgprintf("Channel %08x-%08x in the iplsave.bin is invalid (ret = %d, entry = %d)\n", TITLE_HIGH(tidtmp), TITLE_LOW(tidtmp), result, i);
				retval = 0;
			}
		}
	}
	return retval;
}

void iplsave_init(struct nandfs *fs)
{
	__iplsave_fs = fs;
	struct nandfile *iplsave_file;
	int retval;
	
	iplsave_file = nandfs_open(__iplsave_fs, "/title/00000001/00000002/data/iplsave.bin");
	if (iplsave_file == NULL) {
		dbgprintf("iplsave.bin could not be opened\n");
		return -1;
	}
	// Let them init all they want.
	if(__iplsave_list != NULL)
		free(__iplsave_list);
	__iplsave_size = iplsave_file->entry.size;
	__iplsave_list = calloc(__iplsave_size, 1);
	retval = nandfs_read(__iplsave_list, __iplsave_size, iplsave_file);
	if(retval != __iplsave_size)
		fatal("Couldn't read file!");
	nandfs_close(iplsave_file);
	__iplsave_inited = __check_iplsave_sanity();
}	

iplsave* iplsave_get_list(iplsave* target)
{
	if(__iplsave_inited == 0)
		fatal("iplsave.bin was not inited yet!");
	if(target == NULL)
		fatal("Tried to pass a NULL pointer to iplsave_get_list!");
	memcpy(target, __iplsave_list, __iplsave_size);
	return target;
}

int iplsave_get_size()
{
	return __iplsave_size;
}

int check_channel_validity()
{
	if(__iplsave_inited == 1)
		return WIIFSCK_OK;
	else
		return WIIFSCK_INVAL;
}
