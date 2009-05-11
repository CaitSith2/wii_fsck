#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tools.h"
#include "nand.h"
#include "nandfs.h"
#include "nandstructs.h"
#include "ecc.h"

//#define	NANDFS_DEBUG	1
#ifdef NANDFS_DEBUG
#	define	dbgprintf(f, arg...) do { printf("nandfs: " f, ##arg); } while(0)
#else
#	define	dbgprintf(f, arg...) 
#endif

static s16 nandfs_find_super(rawnand nand)
{
	u32 newest = 0;
	s16 superclstr = 0, cluster;
	struct sffs_header *sffs_head;
	u8 buffer[BYTES_PER_CLUSTER];

	for (cluster = SFFS_START_CLUSTER; cluster < SFFS_END_CLUSTER; cluster++){
		rawnand_read_cluster(nand, buffer, cluster);
		
		sffs_head=(struct sffs_header *)buffer;
		if (memcmp(&sffs_head->fourcc, SFFS_FOURCC, sizeof
					SFFS_FOURCC) == 0) {
//	        fs_hmac_meta(buffer, cluster, calc_hmac);
			
			u32 version = sffs_head->generation;
			if (superclstr == 0 || version > newest) {
				superclstr = cluster;
				newest = version;
			}
		}
	}
	return superclstr;
}

int nandfs_read(void *buffer, size_t size, struct nandfile *fp)
{
	u8 tmpbuf[BYTES_PER_CLUSTER];
	u16 cluster = fp->entry.first_cluster;
	u32 this_size, size_left = size;
	u32 amount_read = 0;
	u32 i = 0;

	if(!fp->enabled)
		fatal("file not opened!\n");

	if (fp->entry.size < size)
		fatal("attempted to read too much from file.\n");

	while (size_left) {
		this_size = size_left > BYTES_PER_CLUSTER ? BYTES_PER_CLUSTER : size_left;

		// Just commenting out for now. This is incredibly redundant.
		// We end up reading and checking each cluster twice. Whee!
		//for (i = 0; i < PAGES_PER_CLUSTER; i++)
		//	rawnand_check_ecc(fp->fs->nand, cluster * PAGES_PER_CLUSTER + i);
		
		rawnand_decrypt_cluster(fp->fs->nand, tmpbuf, cluster);

		memcpy((u8*)(buffer+amount_read), tmpbuf, this_size);
		

		size_left -= this_size;
		amount_read += this_size;
		cluster = be16(&fp->fs->sffs->cluster_map[cluster]);
	}
	return amount_read;
}

int nandfs_write(void *buffer, size_t size, struct nandfile *fp)
{
	u8 tmpbuf[BYTES_PER_CLUSTER];
	u16 cluster = fp->entry.first_cluster;
	u32 this_size, size_left=size;
	u32 amount_wrote = 0;
	u32 i = 0, z = 0;
	u8 ecc[ECC_DATA_SIZE];
	u8 spare[SPARE_DATA_SIZE];
	u8 pagebuf[BYTES_PER_PAGE];

	fatal("this function is not currently at a state where it should be used. Sorry.\n");

	if(!fp->enabled)
		fatal("file not opened!\n");

	if (fp->entry.size < size)
		fatal("attempted to write too much to file.\nWriting more to a file than already exists currently is not supported.\n");

	while (size_left) {
		this_size = size_left > BYTES_PER_CLUSTER ? BYTES_PER_CLUSTER : size_left;

		memcpy(tmpbuf, (u8*)buffer, this_size);
		if (this_size != BYTES_PER_CLUSTER) {
			memset(tmpbuf+this_size, 0, BYTES_PER_CLUSTER-this_size);
		}
		// We still need to update the spare data (except ECC).
		// This function WILL MOST LIKELY BREAK THE FS AT THE MOMENT!
		rawnand_write_encrypted_cluster(fp->fs->nand, tmpbuf, cluster);
		for (i = 0; i < PAGES_PER_CLUSTER; i++)
			rawnand_fix_ecc(fp->fs->nand, cluster * PAGES_PER_CLUSTER + i);

		size_left -= this_size;
		amount_wrote += this_size;
		cluster = fp->fs->sffs->cluster_map[cluster];
	}
	return amount_wrote;
}

static int nandfs_get_fs_entry(struct nandfs *nandfs, size_t dir_levels, char *directories, struct fs_entry *entry)
{
	if (!nandfs->enabled)
		fatal("fs not mounted!\n");

	struct fs_entry _current_dir, *current_dir = &_current_dir;
	memcpy(current_dir, &nandfs->sffs->entries[0],
			sizeof(struct fs_entry));

	while(dir_levels > 0)
	{
		dbgprintf("looking for \"%s\"\n", directories);
		for(;;) {
			// this is the reason i hate structs when you have to take
			// care of different endian systems...
			current_dir->first_child =
				be16((u8 *)&current_dir->first_child);
			current_dir->sibling = be16((u8 *)&current_dir->sibling);
			current_dir->size = be32((u8 *)&current_dir->size);
			current_dir->user_id = be32((u8 *)&current_dir->user_id);
			current_dir->group_id = be16((u8 *)&current_dir->group_id);
			current_dir->unknown = be32((u8 *)&current_dir->unknown);

			dbgprintf("checking (sibling = %08x): %s\n", current_dir->sibling, current_dir->name);
			if(dir_levels > 1 &&
					strncmp(current_dir->name, directories,
						MAX_FS_NAME_LEN) == 0 &&
					current_dir->type == TYPE_DIRECTORY &&
					current_dir->first_child != (s16)0xffff) {
				dbgprintf("found dir; first_child = %08x\n",
						current_dir->first_child);
				memcpy(current_dir, &nandfs->sffs->entries[current_dir->first_child],
						sizeof(struct fs_entry));
				directories += MAX_FS_NAME_LEN;
				dir_levels--;
				break;
			}
			else if(dir_levels == 1 &&
					strncmp(current_dir->name,
						directories,
						MAX_FS_NAME_LEN) == 0 &&
					current_dir->type == TYPE_FILE)
			{
				dbgprintf("FOUND\n");
				memcpy(entry, current_dir, sizeof(struct fs_entry));
				return 0;
			}
			else if(current_dir->sibling != (s16)0xffff)
			{
				dbgprintf("going to next sibling %08x\n",
						current_dir->sibling);
				memcpy(current_dir, &nandfs->sffs->entries[current_dir->sibling],
						sizeof(struct fs_entry));
			}
			else
			{
				dbgprintf("no more siblings :/\n");
				return -1;
			}
		}
	}

	return -1;
}

struct nandfile *nandfs_open(struct nandfs *nandfs, const char *path)
{
	struct nandfile *fp = calloc(1, sizeof(struct nandfile));
	char *directories;
	int dir_depth = 0;
	int ret;
	char *pch;
	char *path_new;

	if (!nandfs->enabled)
		fatal("fs not mounted!\n");

	fp->fs = nandfs;
	directories = calloc(16, MAX_FS_NAME_LEN);
	if(directories == NULL)
		fatal("unable to allocate memory for directories buffer");

	path_new = strdup(path);
	pch = strtok(path_new, "/");


	// HAXX
	strcpy(directories, "/");
	dir_depth++;
	while (pch != NULL) { 
		strncpy(&directories[dir_depth * MAX_FS_NAME_LEN], pch, MAX_FS_NAME_LEN);
		dir_depth++;
		pch = strtok (NULL, "/");
	}

	ret = nandfs_get_fs_entry(nandfs, dir_depth, directories, &fp->entry);
	if(ret < 0)
	{
		free(fp);
		fp = NULL;
	}
	else
	{
		fp->enabled = 1;
	}

/*
	for each cluster i in cluster chain:
		fs_hmac_data(cluster_buf,user_id,name,entry_index,entry->unknown,i++,calc_hmac);
*/    

	free(directories);
	return fp;
}

int nandfs_close(struct nandfile *fp)
{
	if (!fp->enabled)
		fatal("file not open!\n");
	free(fp);
	return 0;
}

struct nandfs* nandfs_mount(rawnand nand)
{ 
	struct nandfs* fs = calloc(1, sizeof(struct nandfs));
	int i;
	u8 buffer[BYTES_PER_CLUSTER];
	u8 *sffs_buffer = calloc(16, BYTES_PER_CLUSTER);

	fs->super = nandfs_find_super(nand);

	for (i = 0; i < 16; i++) {
		rawnand_read_cluster(nand, buffer, fs->super+i);
		memcpy(sffs_buffer + (BYTES_PER_CLUSTER * i), buffer, BYTES_PER_CLUSTER);
 	}

	fs->sffs = (struct sffs *)sffs_buffer;
	fs->nand = nand;
	fs->enabled = 1;

	return fs;
}


