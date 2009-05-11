#include <stdlib.h>
#include <string.h>
#include "tools.h"
#include "titles.h"
#include "nandfs.h"
#include "shared1.h"
#include "wii-fsck.h"
#include "uid.h"

#define	TITLES_DEBUG   1
#ifdef TITLES_DEBUG
#	define	dbgprintf(f, arg...) do { printf("titles: " f, ##arg); } while(0)
#else
#	define	dbgprintf(f, arg...) 
#endif


struct nandfs *__titles_fs = NULL;
// doesn't really need to be dynamic
int __certs_inited = 0;
u8 __system_certs[0xA00];

void titles_init(struct nandfs *fs)
{
	__titles_fs = fs;
}

int __load_cert_data() {
	char filepath[] = { "/sys/cert.sys" };
	struct nandfile *cert_file;
	int retval;
	
	cert_file = nandfs_open(__titles_fs, filepath);
	if (cert_file == NULL) {
		dbgprintf("Cert file could not be opened\n");
		return -1;
	}
	
	if(cert_file->entry.size != 0xA00) {
		fatal("Unexpected cert chain size");
	}
	
	retval = nandfs_read(__system_certs, cert_file->entry.size, cert_file);
	if (retval < cert_file->entry.size) {
		fatal("NANDFS not sane:\nFile: %s\nFileSize: %d Read: %d", filepath, cert_file->entry.size, retval);
	}
	
	nandfs_close(cert_file);
	
	__certs_inited = 1;
	return 0;
}

int __check_signed_data(u8 *signed_data, u32 size) {

	if (!__certs_inited) {
		if (__load_cert_data()) {
			return -1;
		}
	}
	if (check_cert_chain(signed_data, size, __system_certs, 0xA00, 0) !=
			0) {
		if (check_cert_chain(signed_data, size, __system_certs,
				0xA00, 1) == 0)
			return TITLE_SIGSTRNCMP; // FIXME: or with SIGINVAL?
		else
			return TITLE_SIGINVAL;
	}
	return 0;
}

int get_signed_tmd_size(u32 title_h, u32 title_l, u32 *size)
{
	char filepath[43];
	struct nandfile *tmd_file;
	
	sprintf(filepath, "/title/%08x/%08x/content/title.tmd", title_h, title_l);
	tmd_file = nandfs_open(__titles_fs, filepath);
	if (tmd_file == NULL) {
		return TITLES_ENOENT;
	}
	*size = tmd_file->entry.size;
	
	return nandfs_close(tmd_file);
}
	
int get_signed_tmd(u32 title_h, u32 title_l, struct signed_tmd *tmdbuf, int *sig_invalid)
{
	char filepath[43];
	struct nandfile *tmd_file;
	int retval;
	u32 i;
	
	sprintf(filepath, "/title/%08x/%08x/content/title.tmd", title_h, title_l);
	tmd_file = nandfs_open(__titles_fs, filepath);
	if (tmd_file == NULL) {
		return TITLES_ENOENT;
	}
	
	retval = nandfs_read(tmdbuf, tmd_file->entry.size, tmd_file);
	if (retval < tmd_file->entry.size) {
		fatal("NANDFS not sane:\nFile: %s\nFileSize: %d Read: %d", filepath, tmd_file->entry.size, retval);
	}
	nandfs_close(tmd_file);

	if(sig_invalid != NULL)
		*sig_invalid = __check_signed_data((u8*)tmdbuf, retval);
	
	// Signature data
	tmdbuf->sig.sigtype = be32((u8 *)&tmdbuf->sig.sigtype);
	// TMD data
	tmdbuf->data.sys_version = be64((u8 *)&tmdbuf->data.sys_version);
	tmdbuf->data.title_id = be64((u8 *)&tmdbuf->data.title_id);
	tmdbuf->data.title_type = be32((u8 *)&tmdbuf->data.title_type);
	tmdbuf->data.group_id = be16((u8 *)&tmdbuf->data.group_id);
	tmdbuf->data.zero = be16((u8 *)&tmdbuf->data.zero);
	tmdbuf->data.region = be16((u8 *)&tmdbuf->data.region);
	tmdbuf->data.access_rights = be32((u8 *)&tmdbuf->data.access_rights);
	tmdbuf->data.title_version = be16((u8 *)&tmdbuf->data.title_version);
	tmdbuf->data.num_contents = be16((u8 *)&tmdbuf->data.num_contents);
	tmdbuf->data.boot_index = be16((u8 *)&tmdbuf->data.boot_index);
	tmdbuf->data.fill3 = be16((u8 *)&tmdbuf->data.fill3);
	
	// TMD contents list
	for (i = 0; i < tmdbuf->data.num_contents; i++) {
		tmdbuf->data.contents[i].cid = be32((u8 *)&tmdbuf->data.contents[i].cid);
		tmdbuf->data.contents[i].index = be16((u8 *)&tmdbuf->data.contents[i].index);
		tmdbuf->data.contents[i].type = be16((u8 *)&tmdbuf->data.contents[i].type);
		tmdbuf->data.contents[i].size = be64((u8 *)&tmdbuf->data.contents[i].size);
	}
	
	return retval;
}

int get_signed_ticket_size(u32 title_h, u32 title_l, u32 *size)
{
	char filepath[30];
	struct nandfile *tik_file;
	
	sprintf(filepath, "/ticket/%08x/%08x.tik", title_h, title_l);
	tik_file = nandfs_open(__titles_fs, filepath);
	if (tik_file == NULL) {
		return TITLES_ENOENT;
	}
	*size = tik_file->entry.size;
	
	return nandfs_close(tik_file);
}
	
int get_signed_ticket(u32 title_h, u32 title_l, struct signed_tik *tikbuf, int *sig_invalid)
{
	char filepath[30];
	struct nandfile *tik_file;
	int retval;
	u32 i;
	
	sprintf(filepath, "/ticket/%08x/%08x.tik", title_h, title_l);
	tik_file = nandfs_open(__titles_fs, filepath);
	if (tik_file == NULL) {
		return TITLES_ENOENT;
	}
	
	retval = nandfs_read(tikbuf, tik_file->entry.size, tik_file);
	if (retval < tik_file->entry.size)
		fatal("NANDFS not sane:\nFile: %s\nFileSize: %d Read: %d", filepath, tik_file->entry.size, retval);
	nandfs_close(tik_file);

	if(sig_invalid != NULL)
		*sig_invalid = __check_signed_data((u8*)tikbuf, retval);
	
	// Signature data
	tikbuf->sig.sigtype = be32((u8 *)&tikbuf->sig.sigtype);
	// tik data
	tikbuf->data.title_id = be64((u8 *)&tikbuf->data.ticket_id);
	tikbuf->data.device_type = be32((u8 *)&tikbuf->data.device_type);
	tikbuf->data.title_id = be64((u8 *)&tikbuf->data.title_id);
	tikbuf->data.access_mask = be16((u8 *)&tikbuf->data.access_mask);
	tikbuf->data.padding = be16((u8 *)&tikbuf->data.padding);
	
	// ticket limits
	for (i = 0; i < 8; i++) {
		tikbuf->data.limits[i].tag = be32((u8 *)&tikbuf->data.limits[i].tag);
		tikbuf->data.limits[i].value = be32((u8 *)&tikbuf->data.limits[i].value);
	}
	
	return retval;
}

struct nandfile *open_tmd_content(struct tmd *tmdbuf, u16 index) {
	char filepath[46];
	
	if (tmdbuf->contents[index].index != index) {
		dbgprintf("TMD isn't sane! Content %d index: %d", tmdbuf->contents[index].index, index);
		return NULL;
	}
	
	if (tmdbuf->contents[index].type & 0x8000) {
		shared1_init(__titles_fs);
		if (get_shared1_filename(filepath, tmdbuf->contents[index].hash)) {
			return NULL;
		}
	} else {
		sprintf(filepath, "/title/%08x/%08x/content/%08x.app", 
		        TITLE_HIGH(tmdbuf->title_id), 
		        TITLE_LOW(tmdbuf->title_id), 
			   tmdbuf->contents[index].cid);
	}
	dbgprintf("Opening path %s\n", filepath);
	return nandfs_open(__titles_fs, filepath);
}

struct nandfile *open_title_content(u32 title_h, u32 title_l, u16 index) {
	struct signed_tmd *tmdbuf;
	struct nandfile *temp = NULL;
	int retval, valid;
	u32 size;
	
	retval = get_signed_tmd_size(title_h, title_l, &size);
	if (retval == TITLES_ENOENT)
		return NULL;
	
	tmdbuf = malloc(size);
	if (tmdbuf == NULL) {
		fatal("Couldn't allocate memory for tmdbuf. Size: %d\n", size);
	}
	
	retval = get_signed_tmd(title_h, title_l, tmdbuf, &valid);
	
	temp = open_tmd_content(&tmdbuf->data, index);
	free(tmdbuf);
	return temp;
}

int check_title_integrity(u32 title_h, u32 title_l){
	s32 *signed_buf;
	u8 *content_buf;
	struct signed_tmd *p_tmd = NULL;
	struct signed_tik *p_tik = NULL;
	struct nandfile *contentfile;
	int retval, invalid, check_codes = 0;
	u8 hashbuf[20];
	u32 size;
	u16 i;
	int uidfound = 0; 
	
	int title_count = uid_get_uid_count();
	uid* chan_list = calloc(title_count, sizeof(uid));
	uid_get_list(chan_list);
	dbgprintf("Number of uids: %d\n", title_count);
	for(i = 0; i < title_count; i++) {
		u64 tidtmp = be64((u8 *)&chan_list[i].title_id);
		u32 tid_h = TITLE_HIGH(tidtmp);
		u32 tid_l = TITLE_LOW(tidtmp);
		if((title_h == tid_h) && (title_l == tid_l))
    {
			uidfound = 1;
		  dbgprintf("%08x-%08x\n", tid_h, tid_l);
    }
	}
	free(chan_list);
	// Ticket
	retval = get_signed_ticket_size(title_h, title_l, &size);
	if (retval == TITLES_ENOENT) {
		check_codes |= TITLE_TIKENOENT;
	} else if (retval < 0) { 
		return retval;
	} else {
	
		signed_buf = malloc(size);
		if (signed_buf == NULL) {
			fatal("Couldn't allocate memory for signed buffer. Size: %d\n", size);
		}
		p_tik = (struct signed_tik*)signed_buf;
		retval = get_signed_ticket(title_h, title_l, p_tik, &invalid);
		if (retval < 0) {
			goto cleanup;
		}
	
		if (invalid) {
			check_codes |= invalid;
		}
	
		/* Insert ticket checks here? 
		   Don't know what they would be completely */
	
	
		free(signed_buf);
		p_tik = NULL;
	}
	
	// TMD
	retval = get_signed_tmd_size(title_h, title_l, &size);
	if (retval == TITLES_ENOENT) {
		// Skip content stuff
		check_codes |= TITLE_TMDENOENT;
		return check_codes;
	} else if (retval < 0) { 
		return retval;
	}
	
	signed_buf = malloc(size);
	if (signed_buf == NULL) {
		fatal("Couldn't allocate memory for signed buffer. Size: %d\n", size);
	}
	
	p_tmd = (struct signed_tmd*)signed_buf;
	retval = get_signed_tmd(title_h, title_l, p_tmd, &invalid);
	if (retval < 0) {
		goto cleanup;
	}
	
	if (invalid) {
		check_codes |= invalid;
	}
	
	if(p_tmd->data.sys_version)
  {
    u64 tidtmp = p_tmd->data.sys_version;
    dbgprintf("Checking Integrity of title %08X-%08X referenced by title %08X-%08X\n",TITLE_HIGH(tidtmp), TITLE_LOW(tidtmp),title_h,title_l);
    retval = check_title_integrity(TITLE_HIGH(tidtmp), TITLE_LOW(tidtmp));
    if(retval)
      return retval;
  }
	
	// TMD content checks. Pull out to another function? meh.
	for (i = 0; i < p_tmd->data.num_contents; i++) {
		contentfile = open_tmd_content(&(p_tmd->data), i);
		if (contentfile == NULL) {
			check_codes |= TITLE_CONTENTENOENT;
		} else {
			content_buf = malloc(contentfile->entry.size);
			if (signed_buf == NULL) {
				fatal("Couldn't allocate memory for content buffer. Size: %d\n", 
				        contentfile->entry.size);
			}
			 
			retval = nandfs_read(content_buf, contentfile->entry.size, contentfile);
			if (retval < contentfile->entry.size) {
				// Maybe we shouldn't *die* when the nandfs isn't sane...
				fatal("NANDFS not sane: FileSize: %d Read: %d", 
				        contentfile->entry.size, retval);
			}
			
			sha(content_buf, contentfile->entry.size, hashbuf);
			if (memcmp(hashbuf, p_tmd->data.contents[i].hash, 20)) {
				check_codes |= TITLE_CONTENTHASH;
				dbgprintf("Hash fail.\n");
			}
			
			free(content_buf);
			nandfs_close(contentfile);
			contentfile = NULL;
		}
	}
	if(!uidfound)
		check_codes |= TITLE_UIDENOENT;
	retval = check_codes;
	cleanup:
	free(signed_buf);
	return retval;
}
