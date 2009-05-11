#ifndef _WIISTRUCTS_H_
#define _WIISTRUCTS_H_

// wiistructs.h: Structure for wii-specific data types

#include "tools.h"


/* Most of this is jacked from es.h,
 I tried to remove most of the typedef crap */
 
#define ES_SIG_RSA4096              0x10000
#define ES_SIG_RSA2048              0x10001
#define ES_SIG_ECC                  0x10002

#define ES_CERT_RSA4096             0
#define ES_CERT_RSA2048             1
#define ES_CERT_ECC                 2

#define ES_KEY_COMMON               4
#define ES_KEY_SDCARD               6


struct sig_rsa2048 {
	u32 sigtype;
	u8 sig[256];
	u8 fill[60];
#ifdef _MSC_VER
};
#elif __GNUC__  //_MSC_VER
} __attribute__((packed));
#endif //__GNUC__

struct sig_rsa4096 {
	u32 sigtype;
	u8 sig[512];
	u8 fill[60];
#ifdef _MSC_VER
};
#elif __GNUC__  //_MSC_VER
} __attribute__((packed));
#endif //__GNUC__

struct tiklimit {
	u32 tag;
	u32 value;
#ifdef _MSC_VER
};
#elif __GNUC__  //_MSC_VER
} __attribute__((packed));
#endif //__GNUC__

struct tikview {
	u32 view;
	u64 ticket_id;
	u32 device_type;
	u64 title_id;
	u16 access_mask;
	u8 reserved[0x3c];
	u8 cidx_mask[0x40];
	u16 padding;
	struct tiklimit limits[8];
#ifdef _MSC_VER
};
#elif __GNUC__  //_MSC_VER
} __attribute__((packed));
#endif //__GNUC__

struct tik {
	char sig_issuer[0x40];
	u8 fill[63]; // Still just fill?
	u8 cipher_title_key[16];
	u8 fill2;
	u64 ticket_id;
	u32 device_type;
	u64 title_id;
	u16 access_mask;
	u8 reserved[0x3c];
	u8 cidx_mask[0x40];
	u16 padding;
	struct tiklimit limits[8];
#ifdef _MSC_VER
};
#elif __GNUC__  //_MSC_VER
} __attribute__((packed));
#endif //__GNUC__

struct signed_tik {
	struct sig_rsa2048 sig;
	struct tik data;
#ifdef _MSC_VER
};
#elif __GNUC__  //_MSC_VER
} __attribute__((packed));
#endif //__GNUC__

struct tmd_content {
	u32 cid;
	u16 index;
	u16 type;
	u64 size;
	u8 hash[20];
#ifdef _MSC_VER
};
#elif __GNUC__  //_MSC_VER
} __attribute__((packed));
#endif //__GNUC__

struct tmd {
	char sig_issuer[0x40];
	u8 version;
	u8 ca_crl_version;
	u8 signer_crl_version;
	u8 fill2;
	u64 sys_version;
	u64 title_id;
	u32 title_type;
	u16 group_id;
	u16 zero; // part of u32 region, but always zero. split due to misalignment
	u16 region;
	u8 ratings[16];
	u8 reserved[42];
	u32 access_rights;
	u16 title_version;
	u16 num_contents;
	u16 boot_index;
	u16 fill3;
	// content records follow
	// C99 flexible array
	struct tmd_content contents[];
#ifdef _MSC_VER
};
#elif __GNUC__  //_MSC_VER
} __attribute__((packed));
#endif //__GNUC__

struct signed_tmd {
	struct sig_rsa2048 sig;
	struct tmd data;
#ifdef _MSC_VER
};
#elif __GNUC__  //_MSC_VER
} __attribute__((packed));
#endif //__GNUC__

struct cert_header {
	char sig_issuer[0x40];
	u32 cert_type;
	char cert_name[64];
	u32 cert_id; //???
#ifdef _MSC_VER
};
#elif __GNUC__  //_MSC_VER
} __attribute__((packed));
#endif //__GNUC__

struct cert_rsa2048 {
	char sig_issuer[0x40];
	u32 cert_type;
	char cert_name[64];
	u32 cert_id;
	u8 modulus[256];
	u32 exponent;
	u8 pad[0x34];
#ifdef _MSC_VER
};
#elif __GNUC__  //_MSC_VER
} __attribute__((packed));
#endif //__GNUC__

struct cert_rsa4096 {
	char sig_issuer[0x40];
	u32 cert_type;
	char cert_name[64];
	u32 cert_id;
	u8 modulus[512];
	u32 exponent;
	u8 pad[0x34];
#ifdef _MSC_VER
};
#elif __GNUC__  //_MSC_VER
} __attribute__((packed));
#endif //__GNUC__

#define TMD_SIZE(x) (((x)->num_contents)*sizeof(tmd_content) + sizeof(tmd))
// Leaving this in for now
#define TMD_CONTENTS(x) ((x)->contents)

#define IS_VALID_SIGNATURE(x) (((*(x))==ES_SIG_RSA2048) || ((*(x))==ES_SIG_RSA4096))

#define SIGNATURE_SIZE(x) (\
	((*(x))==ES_SIG_RSA2048) ? sizeof(sig_rsa2048) : ( \
	((*(x))==ES_SIG_RSA4096) ? sizeof(sig_rsa4096) : 0 ))

#define SIGNATURE_SIG(x) (((u8*)x)+4)

#define IS_VALID_CERT(x) ((((x)->cert_type)==ES_CERT_RSA2048) || (((x)->cert_type)==ES_CERT_RSA4096))

#define CERTIFICATE_SIZE(x) (\
	(((x)->cert_type)==ES_CERT_RSA2048) ? sizeof(cert_rsa2048) : ( \
	(((x)->cert_type)==ES_CERT_RSA4096) ? sizeof(cert_rsa4096) : 0 ))

#define SIGNATURE_PAYLOAD(x) ((void *)(((u8*)(x)) + SIGNATURE_SIZE(x)))

#define SIGNED_TMD_SIZE(x) ( TMD_SIZE((tmd*)SIGNATURE_PAYLOAD(x)) + SIGNATURE_SIZE(x))
#define SIGNED_TIK_SIZE(x) ( sizeof(tik) + SIGNATURE_SIZE(x) )
#define SIGNED_CERT_SIZE(x) ( CERTIFICATE_SIZE((cert_header*)SIGNATURE_PAYLOAD(x)) + SIGNATURE_SIZE(x))

#define STD_SIGNED_TIK_SIZE ( sizeof(tik) + sizeof(sig_rsa2048) )

#define MAX_NUM_TMD_CONTENTS 512

#define MAX_TMD_SIZE ( sizeof(tmd) + MAX_NUM_TMD_CONTENTS*sizeof(tmd_content) )
#define MAX_SIGNED_TMD_SIZE ( MAX_TMD_SIZE + sizeof(sig_rsa2048) )

#endif
