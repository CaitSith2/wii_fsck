#ifndef _IPLSAVE_H_
#define _IPLSAVE_H_

typedef struct
{
	u8 channel_type;		// 03 for normal channels, 01 for disk channel, 00 for no channel
	u8 secondary_type;		// 00 for normal channels; 01 for disk channel, Mii Channel, and Shop Channel
	u8 unknown[4];			// Unknown. All my titles have these set to 00.
	u16 moveable;			// Not really sure, but all titles except disk use 000e, and disk uses 000f.
	// Since the disk chan is the only one unable to be moved, I assume that it means whether it is movable or not.
	u64 title_id;			// Title ID.
} iplsave_entry;
 
typedef struct
{
	char magic[4];			// The magic! It is always "RIPL"
	u32 filesize;			// The size of iplsave.bin. As of now, is always 0x00000340
	u8 unknown[8];			// Unknown. Seems to always be 0x0000000200000000
	iplsave_entry channels[0x30];	// Channels, listed in order of position in Wii Menu
	u8 unknown2[0x20];		// Unknown. Some may be padding.
	u8 md5_sum[0x10];		// MD5 sum of the rest of the file
} iplsave;

int iplsave_get_size();
iplsave* iplsave_get_list(iplsave* target);
void iplsave_init(struct nandfs *fs);
int check_channel_validity();

#endif //_IPLSAVE_H_
