#ifndef __BOOT2_H__
#define __BOOT2_H__

void boot2_load(rawnand nand, int copy);
int boot2_check_signature();
int boot2_check_contents();

#endif
