
#ifndef _LBS_SHA1_H_
#define _LBS_SHA1_H_


typedef struct _lbs_sha1_context
{
    unsigned long state[5];
    unsigned long count[2];
    unsigned char buffer[64];
}lbs_sha1_ctx_t;

#ifdef __cplusplus
extern "C" {
#endif

    void lbs_sha1_hash(
        unsigned long state[5], 
        const unsigned char buffer[64]
        );

    void lbs_sha1_init(
        lbs_sha1_ctx_t* context
        );

    void lbs_sha1_process(
        lbs_sha1_ctx_t* context, 
        const unsigned char* data, 
        unsigned long len
        );

    void lbs_sha1_final(
        unsigned char digest[20], 
        lbs_sha1_ctx_t* context
        );

#ifdef __cplusplus
};
#endif

#endif