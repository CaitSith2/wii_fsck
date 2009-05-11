#ifndef _SHARED1_H_
#define _SHARED1_H_

// Just init once per nand.
int shared1_init(struct nandfs *fs);
// File names will be only 22 characters, including NULL
int get_shared1_filename(char * filename, const u8 hash[20]);
//struct nandfile *open_shared_content(struct nandfs *fs, const u8 hash[20]);

#endif // _SHARED1_H_
