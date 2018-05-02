/*
---------------------------------------------------------------------------
Copyright (c) 1998-2010, Brian Gladman, Worcester, UK. All rights reserved.

The redistribution and use of this software (with or without changes)
is allowed without the payment of fees or royalties provided that:

  source code distributions include the above copyright notice, this
  list of conditions and the following disclaimer;

  binary distributions include the above copyright notice, this list
  of conditions and the following disclaimer in their documentation.

This software is provided 'as is' with no explicit or implied warranties
in respect of its operation, including, but not limited to, correctness
and fitness for purpose.
---------------------------------------------------------------------------
*/

/** @note [JP Norair, 25-Aug-2014] This is an edited version of aes_modes.c
  * from 20/12/2007, but it only implemented the necessary AES modes for 
  * OpenTag/DASH7.  In particular, this is AES-CTR which is used as the base
  * cipher for EAX.  Additionally, any codeblocks specific to platforms not
  * supported by OpenTag (or ever likely to be supported in the future) have
  * been removed for readability purposes.  To compile for other AES modes,
  * use legacy/aes_modes.c instead of this file.
  */



#include <string.h>
#include <assert.h>

#include "oteax/aesopt.h"

#if defined( AES_MODES )

#define BFR_BLOCKS      8

/* These values are used to detect long word alignment in order to */
/* speed up some buffer operations. This facility may not work on  */
/* some machines so this define can be commented out if necessary  */

#define FAST_BUFFER_OPERATIONS

#define lp32(x)         ((uint_32t*)(x))



#define aligned_array(type, name, no, stride) type name[no]
#define aligned_auto(type, name, no, stride)  type name[no]

#define via_cwd(cwd, ty, dir, len)              \
    aligned_auto(unsigned long, cwd, 4, 16);    \
    cwd[1] = cwd[2] = cwd[3] = 0;               \
    cwd[0] = neh_##dir##_##ty##_key(len)


/* test the code for detecting and setting pointer alignment */

AES_RETURN aes_test_alignment_detection(unsigned int n)	/* 4 <= n <= 16 */
{	uint_8t	p[16];
    uint_32t i, count_eq = 0, count_neq = 0;

    if(n < 4 || n > 16) {
        return EXIT_FAILURE;
    }
    
    for(i = 0; i < n; ++i) {
        uint_8t *qf = ALIGN_FLOOR(p + i, n),
                *qh =  ALIGN_CEIL(p + i, n);
        
        if(qh == qf)
            ++count_eq;
        else if(qh == qf + n)
            ++count_neq;
        else
            return EXIT_FAILURE;
    }
    return (count_eq != 1 || count_neq != n - 1 ? EXIT_FAILURE : EXIT_SUCCESS);
}




AES_RETURN aes_mode_reset(aes_encrypt_ctx ctx[1]) {
    ctx->inf.b[2] = 0;
    return EXIT_SUCCESS;
}




AES_RETURN aes_ecb_encrypt(const unsigned char *ibuf, unsigned char *obuf,
                    int len, const aes_encrypt_ctx ctx[1])
{   int nb = len >> 4;

    if(len & (AES_BLOCK_SIZE - 1))
        return EXIT_FAILURE;

    while(nb--) {
        if(aes_encrypt(ibuf, obuf, ctx) != EXIT_SUCCESS)
            return EXIT_FAILURE;
        ibuf += AES_BLOCK_SIZE;
        obuf += AES_BLOCK_SIZE;
    }
    
    return EXIT_SUCCESS;
}




AES_RETURN aes_ecb_decrypt(const unsigned char *ibuf, unsigned char *obuf,
                    int len, const aes_decrypt_ctx ctx[1])
{   int nb = len >> 4;

    if(len & (AES_BLOCK_SIZE - 1))
        return EXIT_FAILURE;

    while(nb--) {
        if(aes_decrypt(ibuf, obuf, ctx) != EXIT_SUCCESS)
            return EXIT_FAILURE;
        ibuf += AES_BLOCK_SIZE;
        obuf += AES_BLOCK_SIZE;
    }

    return EXIT_SUCCESS;
}




///@todo in addition to the basic functions, aes_ecb_...(), aes_ctr_crypt is 
///      probably the only function we need here, isolate it.

#define BFR_LENGTH  (BFR_BLOCKS * AES_BLOCK_SIZE)

AES_RETURN aes_ctr_crypt(const unsigned char *ibuf, unsigned char *obuf,
            int len, unsigned char *cbuf, cbuf_inc ctr_inc, aes_encrypt_ctx ctx[1])
{   unsigned char   *ip;
    int             i, blen, b_pos = (int)(ctx->inf.b[2]);

    uint_8t buf[BFR_LENGTH];

    if (b_pos) {
        memcpy(buf, cbuf, AES_BLOCK_SIZE);
        if(aes_ecb_encrypt(buf, buf, AES_BLOCK_SIZE, ctx) != EXIT_SUCCESS)
            return EXIT_FAILURE;

        while(b_pos < AES_BLOCK_SIZE && len) {
            *obuf++ = *ibuf++ ^ buf[b_pos++];
            --len;
        }

        if(len)
            ctr_inc(cbuf), b_pos = 0;
    }

    while(len) {
        blen = (len > BFR_LENGTH ? BFR_LENGTH : len), len -= blen;

        for(i = 0, ip = buf; i < (blen >> 4); ++i) {
            memcpy(ip, cbuf, AES_BLOCK_SIZE);
            ctr_inc(cbuf);
            ip += AES_BLOCK_SIZE;
        }

        if(blen & (AES_BLOCK_SIZE - 1))
            memcpy(ip, cbuf, AES_BLOCK_SIZE), i++;

        if(aes_ecb_encrypt(buf, buf, i * AES_BLOCK_SIZE, ctx) != EXIT_SUCCESS)
            return EXIT_FAILURE;

        i = 0; ip = buf;
        
#       ifdef FAST_BUFFER_OPERATIONS
        if(!ALIGN_OFFSET( ibuf, 4 ) && !ALIGN_OFFSET( obuf, 4 ) && !ALIGN_OFFSET( ip, 4 ))
            while(i + AES_BLOCK_SIZE <= blen)
            {
                lp32(obuf)[0] = lp32(ibuf)[0] ^ lp32(ip)[0];
                lp32(obuf)[1] = lp32(ibuf)[1] ^ lp32(ip)[1];
                lp32(obuf)[2] = lp32(ibuf)[2] ^ lp32(ip)[2];
                lp32(obuf)[3] = lp32(ibuf)[3] ^ lp32(ip)[3];
                i += AES_BLOCK_SIZE;
                ip += AES_BLOCK_SIZE;
                ibuf += AES_BLOCK_SIZE;
                obuf += AES_BLOCK_SIZE;
            }
        else
#       endif
            while(i + AES_BLOCK_SIZE <= blen) {
                obuf[ 0] = ibuf[ 0] ^ ip[ 0]; obuf[ 1] = ibuf[ 1] ^ ip[ 1];
                obuf[ 2] = ibuf[ 2] ^ ip[ 2]; obuf[ 3] = ibuf[ 3] ^ ip[ 3];
                obuf[ 4] = ibuf[ 4] ^ ip[ 4]; obuf[ 5] = ibuf[ 5] ^ ip[ 5];
                obuf[ 6] = ibuf[ 6] ^ ip[ 6]; obuf[ 7] = ibuf[ 7] ^ ip[ 7];
                obuf[ 8] = ibuf[ 8] ^ ip[ 8]; obuf[ 9] = ibuf[ 9] ^ ip[ 9];
                obuf[10] = ibuf[10] ^ ip[10]; obuf[11] = ibuf[11] ^ ip[11];
                obuf[12] = ibuf[12] ^ ip[12]; obuf[13] = ibuf[13] ^ ip[13];
                obuf[14] = ibuf[14] ^ ip[14]; obuf[15] = ibuf[15] ^ ip[15];
                i += AES_BLOCK_SIZE;
                ip += AES_BLOCK_SIZE;
                ibuf += AES_BLOCK_SIZE;
                obuf += AES_BLOCK_SIZE;
            }

        while(i++ < blen)
            *obuf++ = *ibuf++ ^ ip[b_pos++];
    }

    ctx->inf.b[2] = (uint_8t)b_pos;
    return EXIT_SUCCESS;
}

#endif
