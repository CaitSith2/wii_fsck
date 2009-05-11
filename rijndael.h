#ifndef __RIJNDAEL_H__
#define __RIJNDAEL_H__

void aes_decrypt(u8 *iv, u8 *inbuf, u8 *outbuf, unsigned long long len);
void aes_encrypt(u8 *iv, u8 *inbuf, u8 *outbuf, unsigned long long len);
void aes_set_key(u8 *key);

#endif