#ifndef _NANDSTRUCTS_H_
#define _NANDSTRUCTS_H_


#include "tools.h"

#define SFFS_FOURCC		("SFFS")

#define MAX_CLUSTERS		(32768)
#define MAX_FS_ENTRIES		(0x17ff)

#define MAX_FS_NAME_LEN		(12)

#define FS_NO_SIBLING		(-1)


#define BYTES_PER_PAGE		(2048)
#define BYTES_PER_CLUSTER	(BYTES_PER_PAGE * PAGES_PER_CLUSTER)
#define BYTES_PER_BLOCK		(BYTES_PER_CLUSTER * CLUSTERS_PER_BLOCK)

#define PAGES_PER_CLUSTER	(8)
#define CLUSTERS_PER_BLOCK	(8)

#define PAGES_PER_BLOCK		(PAGES_PER_CLUSTER * CLUSTERS_PER_BLOCK)
#define SPARE_DATA_SIZE		(64)
#define ECC_DATA_SIZE		(16)
#define HMAC_DATA_SIZE		(48)

#define BLOCK_COUNT		(4096)
#define CLUSTER_COUNT		(BLOCK_COUNT * CLUSTERS_PER_BLOCK)
#define PAGE_COUNT		(CLUSTER_COUNT * PAGES_PER_CLUSTER)

#define NAND_DATA_SIZE		(PAGE_COUNT * BYTES_PER_PAGE)
#define NAND_SIZE		(PAGE_COUNT * (BYTES_PER_PAGE + SPARE_DATA_SIZE))

#define SFFS_COUNT		(16)

#define SFFS_FIRST_BLOCK	(4064)
#define SFFS_FIRST_CLUSTER	(SFFS_FIRST_BLOCK * CLUSTERS_PER_BLOCK)
#define SFFS_FIRST_PAGE		(SFFS_FIRST_CLUSTER * PAGES_PER_CLUSTER)
#define SFFS_OFFSET		(SFFS_FIRST_PAGE * BYTES_PER_PAGE)


//Goes from -1 to -5 or -6, so it needs more investigation
#define CLUSTER_FREE		(-2)
#define CLUSTER_RESERVED	(-4)
#define CLUSTER_CHAIN_LAST	(-5)


#define TYPE_FREE		(0)
#define TYPE_FILE		(1)
#define TYPE_DIRECTORY		(2)

#ifdef _MSC_VER
#pragma pack(push, 1)
#endif //_MSC_VER

/*
"attributes" apears to be unused
The sole purpose of "unknown" is possibly padding up to 32/20h size

"first_child" is used in directiory entries
"first_cluster" is used in file entries
*/

struct fs_entry {
	s8 name[MAX_FS_NAME_LEN];		//00
	union {
		u8 permissions;			//0C
		struct {
			u8 type		: 2;	//0C
			u8 other_perm	: 2;	//0C
			u8 group_perm	: 2;	//0C
			u8 user_perm	: 2;	//0C
		};
	};
	u8 attributes;				//0D
	union {
		s16 first_child;		//0E
		s16 first_cluster;		//0E
	};
	s16 sibling;				//10
	u32 size;				//12
	u32 user_id;				//16
	u16 group_id;				//1A
	u32 unknown;				//1C
#ifdef _MSC_VER
};
#elif __GNUC__  //_MSC_VER
} __attribute__((packed));
#endif //__GNUC__


/*
The highest generation out of the 16 stored SFFS blocks is the newest.
Upon each filesystem modification, the generation number is incremented
and the SFFS block is copied to the next slot (round-robin), for purposes
of data integrity and wear-levelling.  The generation number is then
stored in the Hollywood SEEPROM, but this may or may not actually get used.
*/

struct sffs_header {
	u32 fourcc;
	u32 generation;
	u32 whatever;
#ifdef _MSC_VER
};
#elif __GNUC__  //_MSC_VER
} __attribute__((packed));
#endif //__GNUC__

struct sffs {
	struct sffs_header header;

	// this either is s16 or MAX_CLUSTERS/2
	s16 cluster_map[MAX_CLUSTERS];
	union {
		struct fs_entry entries[MAX_FS_ENTRIES];
		u8 entries_size[0x2FFF4];
	};
#ifdef _MSC_VER
};
#elif __GNUC__  //_MSC_VER
} __attribute__((packed));
#endif //__GNUC__


#ifdef _MSC_VER
#pragma pack(pop)
#endif //_MSC_VER

#endif //_NANDSTRUCTS_H_

