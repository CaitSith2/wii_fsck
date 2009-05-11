#ifndef _NANDFS_H_
#define _NANDFS_H_

// Functions to access the NAND filesystem

#include <stdio.h>
#include "tools.h"
#include "nandstructs.h"
#include "nand.h"

#define SFFS_START_CLUSTER	0x7F00
#define SFFS_END_CLUSTER	0x7FFF

struct nandfile
{
	struct nandfs *fs;
	struct fs_entry entry;
	s8 enabled;
};

struct nandfs
{
	rawnand nand;
	struct sffs *sffs;
	s16 super;
	s8 enabled;
};

struct nandfs *nandfs_mount(rawnand nand);
struct nandfile *nandfs_open(struct nandfs *nandfs, const char *path);
int nandfs_read(void *buffer, size_t size, struct nandfile *fp);
int nandfs_write(void *buffer, size_t size, struct nandfile *fp);
// int nandfs_seek(struct nandfile *fp, u64 offset, u32 whence);
int nandfs_close(struct nandfile *fp);

#endif // _NANDFS_H_

