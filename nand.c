#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tools.h"
#include "nand.h"
#include "ecc.h"

rawnand rawnand_open(char *filename)
{
  int i;
	FILE *nand = fopen(filename, "r+b");

	if(nand == NULL)
		fatal("unable to open nand dump");

	fseek(nand, 0, SEEK_END);
  i = ftell(nand);
	if ((i != 553648128) && (i != 553649152)) {
		printf("WARNING: file size %li is incorrect, are you sure", ftell(nand));
		printf("         your dump contains ECC data?\n");
	}
	fseek(nand, 0, SEEK_SET);
	return nand;
}

void rawnand_close(rawnand nand)
{
	fclose(nand);
}

void rawnand_read_otp(rawnand nand, u8 *buffer)
{
  if(fseek(nand,553648128 + 256,SEEK_SET) == -1)
    fatal("error seeking to OTP data of backupmii nand dump");
  if(fread(buffer, 128, 1, nand) != 1)
    fatal("error reading OTP data of backupmii nand dump");
}

void rawnand_read_spare_data(rawnand nand, u8 *buffer, unsigned int page)
{
	if (fseek(nand, (page * (BYTES_PER_PAGE + SPARE_DATA_SIZE) + BYTES_PER_PAGE), SEEK_SET) == -1)
		fatal("error seeking to spare data of page %d", page);
	if (fread(buffer, SPARE_DATA_SIZE, 1, nand) != 1)
		fatal("error reading spare data for page %d", page);
}

void rawnand_read_page(rawnand nand, u8 *buffer, unsigned int page)
{
	u8 tmpbuf[BYTES_PER_PAGE+SPARE_DATA_SIZE];
	if (fseek(nand, (page * (BYTES_PER_PAGE + SPARE_DATA_SIZE)), SEEK_SET) == -1)
		fatal("error seeking to page %d", page);
	if (fread(tmpbuf, BYTES_PER_PAGE+SPARE_DATA_SIZE, 1, nand) != 1)
		fatal("error reading page %d", page);
	if (check_ecc(tmpbuf) == -1) {
		printf("ECC bad for page %x\n", page);
	}/* else {
		printf("ECC ok for page %x\n", page);
	}*/
	memcpy(buffer, tmpbuf, BYTES_PER_PAGE);
}

void rawnand_read_cluster(rawnand nand, u8 *buffer, unsigned int cluster)
{
	int i;
	for (i = 0; i<PAGES_PER_CLUSTER; i++) {
		rawnand_read_page(nand, buffer + (i * BYTES_PER_PAGE),
			(cluster * PAGES_PER_CLUSTER) + i);
	}
}

void rawnand_read_block(rawnand nand, u8 *buffer, unsigned int block)
{
	int i;
	for (i = 0; i < CLUSTERS_PER_BLOCK; i++) {
		rawnand_read_cluster(nand, buffer + (i * BYTES_PER_CLUSTER),
			(block * CLUSTERS_PER_BLOCK) + i);
	}
}

void rawnand_decrypt_cluster(rawnand nand, u8* buffer, unsigned int cluster)
{
	u8 crypted_buffer[BYTES_PER_CLUSTER];
	u8 iv[16];

	rawnand_read_cluster(nand, crypted_buffer, cluster);

	memset(iv, 0, sizeof iv);
	aes_cbc_dec(get_keys()->nand_key, iv, crypted_buffer, BYTES_PER_CLUSTER, buffer);
}

void rawnand_decrypt_block(rawnand nand, u8 *buffer, unsigned int block)
{
	int i;
	for (i = 0; i < CLUSTERS_PER_BLOCK; i++) {
		rawnand_decrypt_cluster(nand, buffer + (i * BYTES_PER_CLUSTER),
			(block * CLUSTERS_PER_BLOCK) + i);
	}
}

void rawnand_write_spare_data(rawnand nand, u8 *buffer, unsigned int page)
{
	if (fseek(nand, (page * (BYTES_PER_PAGE + SPARE_DATA_SIZE) + BYTES_PER_PAGE), SEEK_SET) == -1)
		fatal("error seeking to spare data of page %d", page);
	if (fwrite(buffer, SPARE_DATA_SIZE, 1, nand) != 1)
		fatal("error writing spare data for page %d", page);
	
}

void rawnand_write_page(rawnand nand, u8 *buffer, unsigned int page)		// EXPECTS A FULL PAGE!
{
	if (fseek(nand, (page * (BYTES_PER_PAGE + SPARE_DATA_SIZE)), SEEK_SET) == -1)
		fatal("error seeking to page %d", page);
	if (fwrite(buffer, BYTES_PER_PAGE, 1, nand) != 1)
		fatal("error writing page %d", page);
	u8 * pagebuf = page * (BYTES_PER_PAGE + SPARE_DATA_SIZE);
	u8 spare[SPARE_DATA_SIZE];
	u8 * ecc = spare + 48;
	rawnand_read_spare_data(nand, spare, page);
	calc_ecc(pagebuf, ecc);
	calc_ecc(pagebuf + 512, ecc + 4);
	calc_ecc(pagebuf + 1024, ecc + 8);
	calc_ecc(pagebuf + 1536, ecc + 12);
	rawnand_write_spare_data(nand, spare, page);
}

void rawnand_write_cluster(rawnand nand, u8 *buffer, unsigned int cluster)
{
	int i;
	for (i = 0; i < PAGES_PER_CLUSTER; i++) {
		rawnand_write_page(nand, buffer + (i * BYTES_PER_PAGE),
			(cluster * PAGES_PER_CLUSTER) + i);
	}
}

void rawnand_write_block(rawnand nand, u8 *buffer, unsigned int block)
{
	int i;
	for (i = 0; i < CLUSTERS_PER_BLOCK; i++) {
		rawnand_write_cluster(nand, buffer + (i * BYTES_PER_CLUSTER),
			(block * CLUSTERS_PER_BLOCK) + i);
	}
}

void rawnand_write_encrypted_cluster(rawnand nand, u8* buffer, unsigned int cluster)
{
	u8 crypted_buffer[BYTES_PER_CLUSTER];
	u8 iv[16];

	if (cluster < FS_START_CLUSTER || cluster > FS_END_CLUSTER) {
		fatal("Cluster %08x should not be written encrypted!", cluster);
	}
	memset(iv, 0, sizeof iv);
	aes_cbc_dec(get_keys()->nand_key, iv, buffer, BYTES_PER_CLUSTER, crypted_buffer);

	rawnand_write_cluster(nand, crypted_buffer, cluster);
}

void rawnand_write_encrypted_block(rawnand nand, u8 *buffer, unsigned int block)
{
	int i;

	if (block < FS_START_BLOCK || block > FS_END_BLOCK) {
		fatal("Block %08x should not be written encrypted!", block);
	}
	for (i = 0; i < CLUSTERS_PER_BLOCK; i++) {
		rawnand_write_encrypted_cluster(nand, buffer + (i * BYTES_PER_CLUSTER),
			(block * CLUSTERS_PER_BLOCK) + i);
	}
}

void rawnand_check_ecc(rawnand nand, unsigned int page)
{
	u8 pagebuf[BYTES_PER_PAGE + SPARE_DATA_SIZE];
	rawnand_read_page(nand, pagebuf, page);
	rawnand_read_spare_data(nand, pagebuf + BYTES_PER_PAGE, page);
	if (check_ecc(pagebuf) == -1) {
		printf("ECC bad for page %x\n", page);
	} else {
		printf("ECC ok for page %x\n", page);
	}
}

void rawnand_fix_ecc(rawnand nand, unsigned int page)
{
	u8 spare[SPARE_DATA_SIZE];
	u8 pagebuf[BYTES_PER_PAGE];
	u8 ecc[ECC_DATA_SIZE];
	memset(spare, 0, SPARE_DATA_SIZE);
	rawnand_read_page(nand, pagebuf, page);
	calc_ecc(pagebuf, ecc);
	calc_ecc(pagebuf + 512, ecc + 4);
	calc_ecc(pagebuf + 1024, ecc + 8);
	calc_ecc(pagebuf + 1536, ecc + 12);
	rawnand_read_spare_data(nand, spare, page);
	memcpy(spare + 48, ecc, ECC_DATA_SIZE);
	rawnand_write_spare_data(nand, spare, page);
}

