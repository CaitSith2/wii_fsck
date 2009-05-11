#include <stdlib.h>
#include <string.h>
#include "tools.h"
#include "nandfs.h"
#include "shared1.h"
#include "uid.h"

// This whole chunk REQUIRES uid.sys be sane.
// If it isn't, you are fucked here.

//#define UID_DEBUG
#ifdef UID_DEBUG
#	define	dbgprintf(f, arg...) do { printf("uid: " f, ##arg); } while(0)
#else
#	define	dbgprintf(f, arg...) 
#endif

struct nandfs *__uid_fs = NULL;
uid *__uid_list = NULL;
int __uids_inited = 0;
int __uid_size = 0;

void uid_init(struct nandfs *fs)
{
	__uid_fs = fs;
	struct nandfile *uid_file;
	int retval;
	
	uid_file = nandfs_open(__uid_fs, "/sys/uid.sys");
	if (uid_file == NULL) {
		dbgprintf("UID file could not be opened\n");
		return -1;
	}
	// Let them init all they want.
	if(__uid_list != NULL)
		free(__uid_list);
	__uid_size = uid_file->entry.size;
	int entry_count = __uid_size / sizeof(uid);
	__uid_list = calloc(__uid_size, 1);
	u8* uid_tmplist = calloc(__uid_size, 1);
	retval = nandfs_read(uid_tmplist, __uid_size, uid_file);
	int i;
	for(i = 0; i < entry_count; i++) {
		__uid_list[i].title_id = *((u64*)&(uid_tmplist[0 + (i * 12)]));
		__uid_list[i].padding = *((u16*)&(uid_tmplist[8 + (i * 12)]));
		__uid_list[i].uid = *((u16*)&(uid_tmplist[10 + (i * 12)]));
	}
	if(retval != __uid_size)
		fatal("Couldn't read file!");
	nandfs_close(uid_file);
	
	__uids_inited = 1;
}	

uid* uid_get_list(uid* target)
{
	if(__uids_inited == 0)
		fatal("UIDs was not inited yet!");
	if(target == NULL)
		fatal("Tried to pass a NULL pointer to uid_get_list!");
	memcpy(target, __uid_list, __uid_size);
	return target;
}

int uid_get_uid_count()
{
	if(__uids_inited == 0)
		fatal("UIDs was not inited yet!");
	return __uid_size / sizeof(uid);
}
