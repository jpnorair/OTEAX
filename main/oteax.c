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
Issue Date: 21/07/2009

 This code implements the EAX combined encryption and authentication mode
 specified M. Bellare, P. Rogaway and D. Wagner.

 This is a byte oriented version in which the nonce is of limited length
*/

#include "oteax.h"
#include "oteax/mode_hdr.h"
#include "oteax/aesopt.h"

//#define OTEAX_TEST_INITKEY
//#define OTEAX_TEST_INITMSG
#define OTEAX_TEXT_CRYPT

#if defined(OTEAX_TEST_INITKEY) \
 || defined(OTEAX_TEST_INITMSG) \
 || defined(OTEAX_TEST_CRYPT)
#   include <stdio.h>
#endif

#if defined(__cplusplus)
extern "C"
    {
#endif

#define BLOCK_SIZE      AES_BLOCK_SIZE      /* block length                 */
#define BLK_ADR_MASK    (BLOCK_SIZE - 1)    /* mask for 'in block' address  */

///@note it seems plausible to do in 32bit alignment, but not guaranteed
#if defined(__ALIGN32__)
//#   define inc_ctr(x)  \
//        {   int i = (BLOCK_SIZE/4); while(i-- > 0 && !++(UI32_PTR(x)[i])) ; }
//#   define dec_ctr(x)  \
//        {   int i = (BLOCK_SIZE/4); while(i-- > 0 && !(UI32_PTR(x)[i])--) ; }
        
#   define inc_ctr(x)   \
    do {    \
        for (int zz=(BLOCK_SIZE/4)-1; zz>=0; zz--) {            \
            x[zz] += NET_ENDIAN32(0x00000001);    if (bval(x[zz],3) == 0) break;   \
            x[zz] += NET_ENDIAN32(0x00000100);    if (bval(x[zz],2) == 0) break;   \
            x[zz] += NET_ENDIAN32(0x00010000);    if (bval(x[zz],1) == 0) break;   \
            x[zz] += NET_ENDIAN32(0x01000000);    if (bval(x[zz],0) == 0) break;   \
        }   \
    } while (0)
    
        
#   define dec_ctr(x)   \
    do {    \
        for (int zz=(BLOCK_SIZE/4)-1; zz>=0; zz--) {            \
            if (bval(x[zz],3) == 0) break;   x[zz] -= NET_ENDIAN32(0x00000001);    \
            if (bval(x[zz],2) == 0) break;   x[zz] -= NET_ENDIAN32(0x00000100);    \
            if (bval(x[zz],1) == 0) break;   x[zz] -= NET_ENDIAN32(0x00010000);    \
            if (bval(x[zz],0) == 0) break;   x[zz] -= NET_ENDIAN32(0x01000000);    \
        }   \
    } while (0)
        
#elif defined(__C2000__)
#   define inc_ctr(x)   do {    \
                            for (int i=BLOCK_SIZE-1; i>=0; i--) {      \
                                __byte((int*)x, i) += 1;            \
                                if ( __byte((int*)x, i) == 0 ) {    \
                                    break;                          \
                                }                                   \
                            }   \
                        while (0)

#   define dec_ctr(x)   do {    \
                            for (int i=BLOCK_SIZE-1; i>=0; i--) {      \
                                if ( __byte((int*)x, i) == 0 ) {    \
                                    break;                          \
                                }                                   \
                                __byte((int*)x, i) -= 1;            \
                            }   \
                        while (0)
                        
#else
#   define inc_ctr(x)  \
        {   int i = BLOCK_SIZE; while(i-- > 0 && !++(UI8_PTR(x)[i])) ; }
#   define dec_ctr(x)  \
        {   int i = BLOCK_SIZE; while(i-- > 0 && !(UI8_PTR(x)[i])--) ; }
#endif



#if defined(__C2000__) || defined(__ALIGN32__)
#   define ALIGN_LENGTH(X)  ((X+3)>>2)
#else
#   define ALIGN_LENGTH(X)  (X)
#endif



/** High-Level (User-Level) Encryption and Decryption routines for EAX
  * ========================================================================<BR>
  * - eax_encrypt_message()
  * - eax_decrypt_message()
  * - eax_init_and_key()
  */

ret_type eax_encrypt_message(const void* iv_v, void* msg_v, unsigned long msg_len, eax_ctx ctx[1]) {
    ///@note [JPN] Tag is always dealt-with as the data right after the message
    unsigned long   aligned_msglen  = ALIGN_LENGTH(msg_len);
    io_t*           tag             = &((io_t*)msg_v)[aligned_msglen];

    eax_init_message((const io_t*)iv_v, ctx);
    eax_encrypt((io_t*)msg_v, aligned_msglen, ctx);
    return eax_compute_tag(tag, ctx);
}



ret_type eax_decrypt_message(const void* iv_v, void* msg_v, unsigned long msg_len, eax_ctx ctx[1]) {   
    ret_type    rr;

    ///@note [JPN] Tag is always dealt-with as the data right after the message
#   if defined(__ALIGN32__) || defined(__C2000__)
    io_t local_tag[1];
    io_t tag[1];
#   else
    io_t local_tag[4];
    io_t tag[4];
#   endif
    
    msg_len = ALIGN_LENGTH(msg_len);
    
    ///@todo Cortex-M supports non-aligned memory access.  I need to test it.
#   if defined (__UNALIGNED_ACCESS__)
        *((uint_32t*)tag) = *((uint_32t*)(msg + msg_len)); 
#   elif defined(__ALIGN32__) || defined(__C2000__)
        tag[0] = ((io_t*)msg_v)[msg_len];
#   else
    {   io_t *cursor;
        cursor  = &((io_t*)msg_v)[msg_len];
        tag[0]  = *cursor++;
        tag[1]  = *cursor++;
        tag[2]  = *cursor++;
        tag[3]  = *cursor;
    }
#   endif
    
    // 1st pass decryption
    eax_init_message(iv_v, ctx);
    eax_decrypt((io_t*)msg_v, msg_len, ctx);
    
    // 2nd pass tag computation and comparison
    rr  = eax_compute_tag(local_tag, ctx);
    rr -= ( *(uint_32t*)tag != *(uint_32t*)local_tag );
    return rr;
}


ret_type eax_init_and_key(const void* key_v, eax_ctx ctx[1]) {
    uint_32t i;
    io_t *p;
#   if defined(__C2000__) || defined(__ALIGN32__)
    uint32_t t;
    static uint_32t x_t[1] = { NET_ENDIAN32(0x00870e87 ^ 0x0000000e) };
#   else
    uint_8t t;
    static uint_8t x_t[4] = { 0x00, 0x87, 0x0e, 0x87 ^ 0x0e };
#   endif

    /* set the context to all zeroes            */
    memset(ctx, 0, sizeof(eax_ctx));

    /* set the AES key                          */
    //aes_encrypt_key(key, key_len, ctx->aes);
    aes_encrypt_key(IO_PTR(key_v), 16, ctx->aes);

    /* compute E(0) (needed for the pad values) */
    aes_encrypt(IO_PTR(ctx->pad_xvv), IO_PTR(ctx->pad_xvv), ctx->aes);

    /* compute {02} * {E(0)} and {04} * {E(0)}  */
    /* GF(2^128) mod x^128 + x^7 + x^2 + x + 1  */
#   if defined(__ALIGN32__)
    p   = IO_PTR(ctx->pad_xvv);
    t   = bval(p[0], 0) >> 6;
    for (i=0; i<(EAX_BLOCK_SIZE/4); ++i) {
        uint_8t m, n;
        uint_8t b0, b1, b2, b3;
        uint_8t b16, b17, b18, b19;

#       ifdef OTEAX_TEST_INITKEY
        printf("Input: %08X -- %02X %02X %02X %02X\n", p[i], bval(p[i], 0), bval(p[i], 1), bval(p[i], 2), bval(p[i], 3));
#       endif

        m   = bval(p[i], 0);
        n   = bval(p[i], 1);
        b0  = ((m << 1) | (n >> 7)) & 0xFF;
        b16 = ((m << 2) | (n >> 6)) & 0xFF;

        m   = n;
        n   = bval(p[i], 2);
        b1  = ((m << 1) | (n >> 7)) & 0xFF;
        b17 = ((m << 2) | (n >> 6)) & 0xFF;
        
        m   = n;
        n   = bval(p[i], 3);
        b2  = ((m << 1) | (n >> 7)) & 0xFF;
        b18 = ((m << 2) | (n >> 6)) & 0xFF;
        
        if (i != ((EAX_BLOCK_SIZE/4)-1)) {
            m   = n;
            n   = bval(p[i+1], 0);
            b3  = ((m << 1) | (n >> 7)) & 0xFF;
            b19 = ((m << 2) | (n >> 6)) & 0xFF;
        }
        else {
            b19     = ((n << 2) ^ bval(x_t[0], t)) & 0xFF;     
            b18    ^= (t >>= 1);
            b3      = (n << 1) ^ bval(x_t[0], t);
        }
        
        p[i]    = bytes2word(b0, b1, b2, b3);
        p[i+4]  = bytes2word(b16, b17, b18, b19);
        
#       ifdef OTEAX_TEST_INITKEY
        printf("Output: %08X -- %02X %02X %02X %02X\n", p[i], b0, b1, b2, b3);
        printf("Output: %08X -- %02X %02X %02X %02X\n\n", p[i+4], b16, b17, b18, b19);
#       endif
    }

    // This version uses C2000 byte intrinsic.  Not considered 32bit Clean.
#   elif defined(__C2000__)
    p   = IO_PTR(ctx->pad_xvv);
    t   = __byte(p, 0) >> 6;
    for(i=0; i<EAX_BLOCK_SIZE-1; ++i) {
        io_t a0 = __byte(p, i);
        io_t b0 = __byte(p, i+1);
        io_t a1 = ((a0 << 1) | (b0 >> 7)) & 0xFF;
        io_t b1 = ((a0 << 2) | (b0 >> 6)) & 0xFF;
        
        __byte(p, i)    = a1;
        __byte(p, i+16) = b1;
    }
    {   
        __byte(p,i+16)  = ((__byte(p,i) << 2) ^ __byte(x,t)) & 0xFF;
        __byte(p,i+15)  = __byte(p,i+15) ^ (t >>= 1);
        __byte(p,i)     = (__byte(p,i) << 1) ^ __byte(x,t);
    }
    
#   else
    p=UI8_PTR(ctx->pad_xvv);
    t=(*p>>6);
    for(i=0; i<EAX_BLOCK_SIZE-1; ++i, ++p) {
#       ifdef OTEAX_TEST_INITKEY
        printf("Input: %02X \n", *p);
#       endif
    
        *(p+16) = (*p<<2) | (*(p+1) >> 6);
        *p      = (*p<<1) | (*(p+1) >> 7);

#       ifdef OTEAX_TEST_INITKEY
        printf("Output: %02X\n", *p);
        printf("Output: %02X\n\n", *(p+16));
#       endif
    }
#   ifdef OTEAX_TEST_INITKEY
    printf("Input: %02X \n", *p);
#   endif
    *(p+16)  = (*p << 2) ^ x_t[t];
    *(p+15) ^= (t >>= 1);
    *p       = (*p << 1) ^ x_t[t];
#   ifdef OTEAX_TEST_INITKEY
    printf("Output: %02X\n", *p);
    printf("Output: %02X\n\n", *(p+15));
    printf("Output: %02X\n\n", *(p+16));
#   endif
    
#   endif

    
    // Test print-out of pad_xvv
    // Requires byte-addressable machine
#   ifdef OTEAX_TEST_INITKEY
    {   for (i=0; i<sizeof(eax_dbuf_t); ) {
            printf("%02X ", ((uint8_t*)ctx->pad_xvv)[i++]);
            if ((i % 16) == 0) {
                printf("\n");
            }
        }
        if ((i % 16) != 0) {
            printf("\n\n");
        }
        else {
            printf("\n");
        }
    }
#   endif

    return RETURN_GOOD;
}





/** Low-Level (Expert-Only) routines for EAX
  * ========================================================================<BR>
  */

ret_type eax_init_message(const io_t* iv, eax_ctx ctx[1]) {
#if defined(__ALIGN32__)
#   define _EAX_BLKSZ   (EAX_BLOCK_SIZE/4)
#else
#   define _EAX_BLKSZ   EAX_BLOCK_SIZE
#endif
    uint_32t i = 0;
    uint_32t n_pos = 0;
    io_t *p;

    /* Initialize nonce and cipher-text block buffers */
    memset(ctx->nce_cbc, 0, EAX_BLOCK_SIZE);
    memset(ctx->txt_cbc, 0, EAX_BLOCK_SIZE);

    /* set the ciphertext CBC start value       */
#   if defined(__ALIGN32__)
        ctx->txt_cbc[_EAX_BLKSZ-1] = NET_ENDIAN32(0x00000002);
#   elif defined(__C2000__)
        __byte(ctx->txt_cbc, EAX_BLOCK_SIZE-1) = 2;
#   else
        UI8_PTR(ctx->txt_cbc)[_EAX_BLKSZ - 1] = 2;
#   endif
    ctx->txt_ccnt = 0;  /* encryption count     */
    ctx->txt_acnt = 0;  /* authentication count */

    /* compile the OMAC value for the nonce     */
#   if defined(__ALIGN32__)
    n_pos = 7;      ///@todo inspect i and n_pos usage below
    aes_encrypt(IO_PTR(ctx->nce_cbc), IO_PTR(ctx->nce_cbc), ctx->aes);
    ctx->nce_cbc[0] ^= iv[0];
    ctx->nce_cbc[1] ^= (iv[1] & NET_ENDIAN32(0xFFFFFF00));
    
#   else // Not 32bit clean
    n_pos = 16;
    i = 0;
    while (i < 7) {
        if (n_pos == EAX_BLOCK_SIZE) {
            aes_encrypt(IO_PTR(ctx->nce_cbc), IO_PTR(ctx->nce_cbc), ctx->aes);
            n_pos = 0;
        }
#       if defined(__C2000__)
            __byte(ctx->nce_cbc, n_pos) ^= __byte(iv, i);
            i++;
            n_pos++;
#       else
            UI8_PTR(ctx->nce_cbc)[n_pos++] ^= iv[i++];
#       endif
    }
#   endif

#   ifdef OTEAX_TEST_INITMSG
    printf("Nonce Stage 1:\n%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\n\n", 
            ((uint8_t*)ctx->nce_cbc)[0], ((uint8_t*)ctx->nce_cbc)[1], ((uint8_t*)ctx->nce_cbc)[2], ((uint8_t*)ctx->nce_cbc)[3], 
            ((uint8_t*)ctx->nce_cbc)[4], ((uint8_t*)ctx->nce_cbc)[5], ((uint8_t*)ctx->nce_cbc)[6], ((uint8_t*)ctx->nce_cbc)[7],
            ((uint8_t*)ctx->nce_cbc)[8], ((uint8_t*)ctx->nce_cbc)[9], ((uint8_t*)ctx->nce_cbc)[10], ((uint8_t*)ctx->nce_cbc)[11],
            ((uint8_t*)ctx->nce_cbc)[12], ((uint8_t*)ctx->nce_cbc)[13], ((uint8_t*)ctx->nce_cbc)[14], ((uint8_t*)ctx->nce_cbc)[15]
        );
    printf("n_pos = %d\n", n_pos);
#   endif

    /* do the OMAC padding for the nonce        */
    ///@todo can probably optimize this for word-align XOR
    p = IO_PTR(ctx->pad_xvv);
    if (n_pos < EAX_BLOCK_SIZE) {
#       if defined(__ALIGN32__)
            // Take advantage of fact that we know for sure: n_pos == 7
            ctx->nce_cbc[1] ^= NET_ENDIAN32(0x00000080);
            p = &p[4];
            
#       elif defined(__C2000__)     // Not 32bit clean
            __byte(ctx->nce_cbc, n_pos) ^= 0x80;
            p = &p[4];
            
#       else                        // Not 32bit clean
            UI8_PTR(ctx->nce_cbc)[n_pos] ^= 0x80;
            p = &UI8_PTR(p)[16];
            
#       endif
    }

#   ifdef OTEAX_TEST_INITMSG
    printf("Nonce Stage 2:\n%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\n\n", 
            ((uint8_t*)ctx->nce_cbc)[0], ((uint8_t*)ctx->nce_cbc)[1], ((uint8_t*)ctx->nce_cbc)[2], ((uint8_t*)ctx->nce_cbc)[3], 
            ((uint8_t*)ctx->nce_cbc)[4], ((uint8_t*)ctx->nce_cbc)[5], ((uint8_t*)ctx->nce_cbc)[6], ((uint8_t*)ctx->nce_cbc)[7],
            ((uint8_t*)ctx->nce_cbc)[8], ((uint8_t*)ctx->nce_cbc)[9], ((uint8_t*)ctx->nce_cbc)[10], ((uint8_t*)ctx->nce_cbc)[11],
            ((uint8_t*)ctx->nce_cbc)[12], ((uint8_t*)ctx->nce_cbc)[13], ((uint8_t*)ctx->nce_cbc)[14], ((uint8_t*)ctx->nce_cbc)[15]
        );
#   endif

    ///@note this has been optimized for word-aligned XOR
    for(i=0; i<_EAX_BLKSZ; ++i) {
#   if defined(__ALIGN32__) || defined(__C2000__)
        UI32_PTR(ctx->nce_cbc)[i] ^= UI32_PTR(p)[i];
#   else
        UI8_PTR(ctx->nce_cbc)[i] ^= p[i];
#   endif
    }
    
#   ifdef OTEAX_TEST_INITMSG
    printf("Nonce Stage 3:\n%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\n\n", 
            ((uint8_t*)ctx->nce_cbc)[0], ((uint8_t*)ctx->nce_cbc)[1], ((uint8_t*)ctx->nce_cbc)[2], ((uint8_t*)ctx->nce_cbc)[3], 
            ((uint8_t*)ctx->nce_cbc)[4], ((uint8_t*)ctx->nce_cbc)[5], ((uint8_t*)ctx->nce_cbc)[6], ((uint8_t*)ctx->nce_cbc)[7],
            ((uint8_t*)ctx->nce_cbc)[8], ((uint8_t*)ctx->nce_cbc)[9], ((uint8_t*)ctx->nce_cbc)[10], ((uint8_t*)ctx->nce_cbc)[11],
            ((uint8_t*)ctx->nce_cbc)[12], ((uint8_t*)ctx->nce_cbc)[13], ((uint8_t*)ctx->nce_cbc)[14], ((uint8_t*)ctx->nce_cbc)[15]
        );
#   endif
    
    /* compute the OMAC*(nonce) value           */
    aes_encrypt(IO_PTR(ctx->nce_cbc), IO_PTR(ctx->nce_cbc), ctx->aes);

#   ifdef OTEAX_TEST_INITMSG
    printf("Nonce Stage 4:\n%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\n\n", 
            ((uint8_t*)ctx->nce_cbc)[0], ((uint8_t*)ctx->nce_cbc)[1], ((uint8_t*)ctx->nce_cbc)[2], ((uint8_t*)ctx->nce_cbc)[3], 
            ((uint8_t*)ctx->nce_cbc)[4], ((uint8_t*)ctx->nce_cbc)[5], ((uint8_t*)ctx->nce_cbc)[6], ((uint8_t*)ctx->nce_cbc)[7],
            ((uint8_t*)ctx->nce_cbc)[8], ((uint8_t*)ctx->nce_cbc)[9], ((uint8_t*)ctx->nce_cbc)[10], ((uint8_t*)ctx->nce_cbc)[11],
            ((uint8_t*)ctx->nce_cbc)[12], ((uint8_t*)ctx->nce_cbc)[13], ((uint8_t*)ctx->nce_cbc)[14], ((uint8_t*)ctx->nce_cbc)[15]
        );
#   endif

    /* copy value into counter for CTR          */
    memcpy(ctx->ctr_val, ctx->nce_cbc, _EAX_BLKSZ);
    return RETURN_GOOD;
    
#undef _EAX_BLKSZ
}





ret_type eax_auth_data(const io_t* data, unsigned long data_len, eax_ctx ctx[1]) {
#if defined(__C2000__) || defined(__ALIGN32__)
#   define _BUFINC  (BUF_INC/4)
#   define _BLKSZ   (BLOCK_SIZE/4)
#   define _BUFMASK ((BUF_INC/4)-1)
#else
#   define _BUFINC  BUF_INC
#   define _BLKSZ   BLOCK_SIZE
#   define _BUFMASK BUF_ADRMASK
#endif

    uint_32t cnt    = 0;
    uint_32t b_pos  = ctx->txt_acnt & (_BLKSZ-1);

    if (!data_len) {
        return RETURN_GOOD;
    }

    if (((data - &(IO_PTR(ctx->txt_cbc))[b_pos]) & _BUFMASK) == 0) {
        if (b_pos != 0) {
            while (cnt < data_len && (b_pos & _BUFMASK)) {
               IO_PTR(ctx->txt_cbc)[b_pos++] ^= data[cnt++];
            }
            while(cnt + _BUFINC <= data_len && b_pos <= _BLKSZ - _BUFINC) {
                *UINT_PTR(&(IO_PTR(ctx->txt_cbc))[b_pos]) ^= *UINT_PTR(&data[cnt]);
                cnt += _BUFINC; 
                b_pos += _BUFINC;
            }
        }
        while (cnt + _BLKSZ <= data_len) {
            aes_encrypt(IO_PTR(ctx->txt_cbc), IO_PTR(ctx->txt_cbc), ctx->aes);
            xor_block_aligned(ctx->txt_cbc, ctx->txt_cbc, &data[cnt]);
            cnt += _BLKSZ;
        }
    }
    ///@note this "else" section will never run when IO is aligned with the
    ///      crypto-compute buffer (32 bit alignment)
#   if !defined(__ALIGN32__) && !defined(__C2000__)
    else {
        if (b_pos != 0) {
            while ((cnt < data_len) && (b_pos < _BLKSZ)) {
               IO_PTR(ctx->txt_cbc)[b_pos++] ^= data[cnt++];
            }
        }
        while ((cnt + _BLKSZ) <= data_len) {
            aes_encrypt(IO_PTR(ctx->txt_cbc), IO_PTR(ctx->txt_cbc), ctx->aes);
            xor_block(ctx->txt_cbc, ctx->txt_cbc, &data[cnt]);
            cnt += _BLKSZ;
        }
    }
#   endif

    while (cnt < data_len) {
        if ((b_pos == _BLKSZ) || (b_pos == 0)) {
            aes_encrypt(IO_PTR(ctx->txt_cbc), IO_PTR(ctx->txt_cbc), ctx->aes);
            b_pos = 0;
        }
        IO_PTR(ctx->txt_cbc)[b_pos++] ^= data[cnt++];
    }

    ctx->txt_acnt += cnt;
    return RETURN_GOOD;

#undef _BUFMASK
#undef _BLKSZ
#undef _BUFINC
}




ret_type eax_crypt_data(io_t* data, unsigned long data_len, eax_ctx ctx[1]) {
#if defined(__C2000__) || defined(__ALIGN32__)
#   define _BUFINC  (BUF_INC/4)
#   define _BLKSZ   (BLOCK_SIZE/4)
#   define _ADRMASK (
#   define _BUFMASK ((BUF_INC/4)-1)
#else
#   define _BUFINC  BUF_INC
#   define _BLKSZ   BLOCK_SIZE
#   define _ADRMASK BUF_ADRMASK
#   define _BUFMASK BUF_ADRMASK
#endif

    ///@note b_pos is the buffer position.  EAX is a streaming encryption, so
    ///      technically it could be done in multiple, sequential passes.
    uint_32t cnt    = 0;
    uint_32t b_pos  = ctx->txt_ccnt & _BUFMASK;
    
    if (data_len == 0) {
        return RETURN_GOOD;
    }

    if(((data - &(IO_PTR(ctx->enc_ctr))[b_pos]) & _BUFMASK) == 0) {
        if (b_pos != 0) {
            while (cnt < data_len && (b_pos & _BUFMASK)) {
                data[cnt++] ^= IO_PTR(ctx->enc_ctr)[b_pos++];
              }

            while (cnt + _BUFINC <= data_len && b_pos <= _BLKSZ - _BUFINC) {
                *UINT_PTR(&IO_PTR(data)[cnt]) ^= *UINT_PTR(&IO_PTR(ctx->enc_ctr)[b_pos]);
                cnt += _BUFINC;
                b_pos += _BUFINC;
            }
        }

        while(cnt + _BLKSZ <= data_len) {
            aes_encrypt(IO_PTR(ctx->ctr_val), IO_PTR(ctx->enc_ctr), ctx->aes);
            inc_ctr(ctx->ctr_val);
            xor_block_aligned( &data[cnt], &data[cnt], ctx->enc_ctr);
            cnt += _BLKSZ;
        }
    }
    
    
    ///@note this "else" section will never run when IO is aligned with the
    ///      crypto-compute buffer (32 bit alignment)
#   if !defined(__ALIGN32__) && !defined(__C2000__)
    else {
        if (b_pos != 0)
            while(cnt < data_len && b_pos < _BLKSZ)
                data[cnt++] ^= UI8_PTR(ctx->enc_ctr)[b_pos++];

        while(cnt + _BLKSZ <= data_len) {
            aes_encrypt(UI8_PTR(ctx->ctr_val), UI8_PTR(ctx->enc_ctr), ctx->aes);
            inc_ctr(ctx->ctr_val);
            xor_block(data + cnt, data + cnt, ctx->enc_ctr);
            cnt += _BLKSZ;
        }
    }
#   endif

    while(cnt < data_len) {
        if(b_pos == _BLKSZ || (b_pos == 0)) {
            aes_encrypt(IO_PTR(ctx->ctr_val), IO_PTR(ctx->enc_ctr), ctx->aes);
            b_pos = 0;
            inc_ctr(ctx->ctr_val);
        }
        data[cnt++] ^= IO_PTR(ctx->enc_ctr)[b_pos++];
    }

    ctx->txt_ccnt += cnt;
    return RETURN_GOOD;
    
#undef _BUFMASK
#undef _BLKSZ
#undef _BUFINC
}




ret_type eax_compute_tag(io_t* tag, eax_ctx ctx[1]) {   
#if defined(__C2000__) || defined(__ALIGN32__)
#   define _BLKSZ   (BLOCK_SIZE/4)
#else
#   define _BLKSZ   BLOCK_SIZE
#endif

    uint_32t i;
    io_t *p;

    if ((ctx->txt_acnt != ctx->txt_ccnt) && ctx->txt_ccnt > 0) {
        return RETURN_ERROR;
    }

    /* complete OMAC* for ciphertext value  */
    p = IO_PTR(ctx->pad_xvv);
    i = ctx->txt_acnt & (_BLKSZ-1);
    if (i != 0) {
#   if defined(__C2000__) || defined(__ALIGN32__)
        //__byte(ctx->txt_cbc, i*4) ^= 0x80;
        ctx->txt_cbc[i] ^= NET_ENDIAN32(0x80000000);
        p = &p[4];
#   else
        UI8_PTR(ctx->txt_cbc)[i] ^= 0x80;
        p += 16;
#   endif
    }

    xor_block_aligned(ctx->txt_cbc, ctx->txt_cbc, p);
    aes_encrypt(IO_PTR(ctx->txt_cbc), IO_PTR(ctx->txt_cbc), ctx->aes);

    /* compute final authentication tag     */
    ///@todo Aligned XOR should be possible in any case
#   if defined(__C2000__) || defined(__ALIGN32__)
        tag[0] = UI32_PTR(ctx->nce_cbc)[0] ^ UI32_PTR(ctx->txt_cbc)[0];
#   else
        tag[0] = UI8_PTR(ctx->nce_cbc)[0] ^ UI8_PTR(ctx->txt_cbc)[0];
        tag[1] = UI8_PTR(ctx->nce_cbc)[1] ^ UI8_PTR(ctx->txt_cbc)[1];
        tag[2] = UI8_PTR(ctx->nce_cbc)[2] ^ UI8_PTR(ctx->txt_cbc)[2];
        tag[3] = UI8_PTR(ctx->nce_cbc)[3] ^ UI8_PTR(ctx->txt_cbc)[3];
#   endif
    
    
    ///@note changed to 0 - (ctx->txt_ccnt != ctx->txt_acnt)
    //return (ctx->txt_ccnt == ctx->txt_acnt) ? RETURN_GOOD : RETURN_WARN;
    return 0 - (ctx->txt_ccnt != ctx->txt_acnt);
    
#undef _BUFMASK
#undef _BLKSZ
#undef _BUFINC
}



ret_type eax_end(eax_ctx ctx[1]) {
    memset(ctx, 0, sizeof(eax_ctx));
    return RETURN_GOOD;
}

ret_type eax_encrypt(io_t* data, unsigned long data_len, eax_ctx ctx[1]) {
    eax_crypt_data(data, data_len, ctx);
    eax_auth_data(data, data_len, ctx);
    return RETURN_GOOD;
}

ret_type eax_decrypt(io_t* data, unsigned long data_len, eax_ctx ctx[1]) {
    eax_auth_data(data, data_len, ctx);
    eax_crypt_data(data, data_len, ctx);
    return RETURN_GOOD;
}





#if defined(__cplusplus)
    }
#endif
