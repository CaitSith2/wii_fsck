#ifndef _UID_H_
#define _UID_H_

typedef struct
{
	u64 title_id;
	u16 padding;
	u16 uid;
} uid;

int uid_get_uid_count();
uid* uid_get_list(uid* target);
void uid_init(struct nandfs *fs);

#endif //_UID_H_
