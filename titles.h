#ifndef _TITLES_H_
#define _TITLES_H_

#include "wiistructs.h"
#include "nandfs.h"

#define TITLES_ENOENT           -0x1000

// integrity check bits. names are dumb. Feel free to replace.
#define TITLE_OK               0
#define TITLE_SIGINVAL         1
#define TITLE_TMDENOENT        2
#define TITLE_TIKENOENT        4
#define TITLE_CONTENTHASH      8
#define TITLE_CONTENTENOENT    16
#define	TITLE_SIGSTRNCMP       32
#define TITLE_UIDENOENT        64



#define TITLE_HIGH(x)           ((u32)((x) >> 32))
#define TITLE_LOW(x)            ((u32)(x))

void titles_init(struct nandfs *fs);

int get_signed_tmd_size(u32 title_h, u32 title_l, u32 *size);
int get_signed_tmd(u32 title_h, u32 title_l, struct signed_tmd *tmdbuf, int *signature_invalid);
int get_signed_ticket_size(u32 title_h, u32 title_l, u32 *size);
int get_signed_ticket(u32 title_h, u32 title_l, struct signed_tik *tikbuf, int *signature_invalid);

// Just pass these to nandfs_close() for now.
struct nandfile *open_tmd_content(struct tmd *tmdbuf, u16 index);
struct nandfile *open_title_content(u32 title_h, u32 title_l, u16 index);

int check_title_integrity(u32 title_h, u32 title_l);

#endif // _TITLES_H_
