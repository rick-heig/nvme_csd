#ifndef __AES_CBC_H__
#define __AES_CBC_H__

#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>

void handleErrors(void);
int aes_encrypt(unsigned char *ciphertext, unsigned char *plaintext, int plaintext_len,
                unsigned char *key, unsigned char *iv);
int aes_decrypt(unsigned char *plaintext, unsigned char *ciphertext, int ciphertext_len,
                unsigned char *key, unsigned char *iv);
#if 0 /* Weird example interface with weird direction */
int aes_encrypt(unsigned char *plaintext, int plaintext_len, unsigned char *key,
                unsigned char *iv, unsigned char *ciphertext);
int aes_decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *key,
                unsigned char *iv, unsigned char *plaintext);
#endif

#endif  /* __AES_CBC_H__ */