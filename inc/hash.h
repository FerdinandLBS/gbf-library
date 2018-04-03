#ifndef __HASH_H__
#define __HASH_H__

/* MD5 context. */
typedef struct {
    unsigned state[4];                                   /* state(ABCD) */
    unsigned count[2];        /* number of bits, modulo 2^64(lsb first) */
    unsigned char buffer[64];                         /* input buffer */
} MD5_CTX;

typedef struct _hash_sha1_context
{
    unsigned long state[5];
    unsigned long count[2];
    unsigned char buffer[64];
}SHA1_CTX;

#ifdef __cplusplus
extern "C" {
#endif

    void hash_sha1_hash(
        unsigned long state[5], 
        const unsigned char buffer[64]
    );

    void hash_sha1_init(
        SHA1_CTX* context
        );

    void hash_sha1_process(
        SHA1_CTX* context, 
        const unsigned char* data, 
        unsigned long len
        );

    void hash_sha1_final(
        unsigned char digest[20], 
        SHA1_CTX* context
        );


    void hash_md5_init(MD5_CTX *context);
    void hash_md5_update(MD5_CTX *context, unsigned char *input, unsigned int inputLen);
    void hash_md5_update_string(MD5_CTX *context,const char *string);
    void hash_md5_final(unsigned char digest[16], MD5_CTX *context);
    void hash_md5_string(char *string,unsigned char digest[16]);

#ifdef __cplusplus
};
#endif


#endif