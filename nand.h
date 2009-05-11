#ifndef _NAND_H_
#define _NAND_H_

// Functions to access the NAND image
// boilerplate etc...

#include <stdio.h>
#include "tools.h"
#include "nandstructs.h"

#define RAW_FLASH_DEV	"/dev/flash"

#define FS_START_CLUSTER	0x40
#define FS_END_CLUSTER		0x7F00
#define FS_START_BLOCK		0x8
#define FS_END_BLOCK		0xFE0

// --- raw nand access functions ---
// endianness is handled at a higher level

rawnand rawnand_open(char *filename);
void rawnand_close(rawnand nand);
// consider the following: what types should we use for offset/length?
// in addition, these are seek & read all-in-one functions
// would something more similar to the C file library be better?
// aka fseek() and fread()
// These functions read raw data.
void rawnand_read_otp(rawnand nand, u8 *buffer);
void rawnand_read_spare_data(rawnand nand, u8 *buffer, unsigned int page);
void rawnand_read_page(rawnand nand, u8 *buffer, unsigned int page);
void rawnand_read_block(rawnand nand, u8 *buffer, unsigned int block);
void rawnand_read_cluster(rawnand nand, u8 *buffer, unsigned int cluster);
// These functions read encrypted data, and decrypts it.
void rawnand_decrypt_cluster(rawnand nand, u8* buffer, unsigned int cluster);
void rawnand_decrypt_block(rawnand nand, u8 *buffer, unsigned int block);
// These functions take raw data, and writes it.
void rawnand_write_spare_data(rawnand nand, u8 *buffer, unsigned int page);
void rawnand_write_page(rawnand nand, u8 *buffer, unsigned int page);
void rawnand_write_block(rawnand nand, u8 *buffer, unsigned int block);
void rawnand_write_cluster(rawnand nand, u8 *buffer, unsigned int cluster);
// These functions take unencrypted data, encrypts it, then writes it.
void rawnand_write_encrypted_cluster(rawnand nand, u8* buffer, unsigned int cluster);
void rawnand_write_encrypted_block(rawnand nand, u8 *buffer, unsigned int block);
// These functions deal with the spare data.
void rawnand_check_ecc(rawnand nand, unsigned int page);
void rawnand_fix_ecc(rawnand nand, unsigned int page);

#endif // _NAND_H_

