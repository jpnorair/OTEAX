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

#if defined(__cplusplus)
extern "C"
    {
#endif

#define BLOCK_SIZE      AES_BLOCK_SIZE      /* block length                 */
#define BLK_ADR_MASK    (BLOCK_SIZE - 1)    /* mask for 'in block' address  */


#if !defined(__C2000__)
#   define inc_ctr(x)  \
        {   int i = BLOCK_SIZE; while(i-- > 0 && !++(UI8_PTR(x)[i])) ; }
#   define dec_ctr(x)  \
        {   int i = BLOCK_SIZE; while(i-- > 0 && !(UI8_PTR(x)[i])--) ; }
#else
///@todo write this with __byte
#   define inc_ctr(x)   do {    \
                            for (int i=BLOCK_SIZE; i>0; i--) {      \
                                __byte((int*)x, i) += 1;            \
                                if ( __byte((int*)x, i) == 0 ) {    \
                                    break;                          \
                                }                                   \
                            }   \
                        while (0)

#   define dec_ctr(x)   do {    \
                            for (int i=BLOCK_SIZE; i>0; i--) {      \
                                if ( __byte((int*)x, i) == 0 ) {    \
                                    break;                          \
                                }                                   \
                                __byte((int*)x, i) -= 1;            \
                            }   \
                        while (0)
#endif

ret_type eax_init_and_key(const io_t key[], eax_ctx ctx[1]) {
    static uint_8t x_t[4] = { 0x00, 0x87, 0x0e, 0x87 ^ 0x0e };
    uint_8t t, *p;
    uint_32t i;

    /* set the context to all zeroes            */
    memset(ctx, 0, sizeof(eax_ctx));

    /* set the AES key                          */
    //aes_encrypt_key(key, key_len, ctx->aes);
    aes_encrypt_key(key, 16, ctx->aes);

    /* compute E(0) (needed for the pad values) */
    aes_encrypt(UI8_PTR(ctx->pad_xvv), UI8_PTR(ctx->pad_xvv), ctx->aes);

    /* compute {02} * {E(0)} and {04} * {E(0)}  */
    /* GF(2^128) mod x^128 + x^7 + x^2 + x + 1  */
    for(i=0, p=UI8_PTR(ctx->pad_xvv), t=(*p>>6); i<EAX_BLOCK_SIZE-1; ++i, ++p) {
        *(p+16) = (*p<<2) | (*(p+1) >> 6);
        *p      = (*p<<1) | (*(p+1) >> 7);
    }
    *(p+16)  = (*p << 2) ^ x_t[t];
    *(p+15) ^= (t >>= 1);
    *p       = (*p << 1) ^ x_t[t];

    return RETURN_GOOD;
}



ret_type eax_init_message(const io_t iv[], eax_ctx ctx[1])
{   
    uint_32t i = 0, n_pos = 0;
    uint_8t *p;

    ///@todo update this to word-aligned store
    ///@todo see if it is OK to get rid of the header part
    memset(ctx->nce_cbc, 0, EAX_BLOCK_SIZE);
    //memset(ctx->hdr_cbc, 0, EAX_BLOCK_SIZE);
    memset(ctx->txt_cbc, 0, EAX_BLOCK_SIZE);

    /* set the header CBC start value           */
    ///@todo see if it is OK to get rid of the header part
    //UI8_PTR(ctx->hdr_cbc)[EAX_BLOCK_SIZE - 1] = 1;
    //ctx->hdr_cnt = 0;

    /* set the ciphertext CBC start value       */
    UI8_PTR(ctx->txt_cbc)[EAX_BLOCK_SIZE - 1] = 2;
    ctx->txt_ccnt = 0;  /* encryption count     */
    ctx->txt_acnt = 0;  /* authentication count */

    n_pos = 16;

    /* compile the OMAC value for the nonce     */
    i = 0;
    while(i < 7) {
        if (n_pos == EAX_BLOCK_SIZE) {
            aes_encrypt(UI8_PTR(ctx->nce_cbc), UI8_PTR(ctx->nce_cbc), ctx->aes);
            n_pos = 0;
        }
        UI8_PTR(ctx->nce_cbc)[n_pos++] ^= iv[i++];
    }

    /* do the OMAC padding for the nonce        */
    p = UI8_PTR(ctx->pad_xvv);
    if(n_pos < EAX_BLOCK_SIZE) {
        UI8_PTR(ctx->nce_cbc)[n_pos] ^= 0x80;
        p += 16;
    }

    ///@todo unroll loop and de-index
    for(i = 0; i < EAX_BLOCK_SIZE; ++i)
        UI8_PTR(ctx->nce_cbc)[i] ^= p[i];

    /* compute the OMAC*(nonce) value           */
    aes_encrypt(UI8_PTR(ctx->nce_cbc), UI8_PTR(ctx->nce_cbc), ctx->aes);

    /* copy value into counter for CTR          */
    memcpy(ctx->ctr_val, ctx->nce_cbc, EAX_BLOCK_SIZE);
    return RETURN_GOOD;
}



///@todo this function doesn't need to be optimized because no header is 
///      used with OpenTag's variant of EAX
ret_type eax_auth_header(const io_t hdr[], unsigned long hdr_len, eax_ctx ctx[1])
{   
/*
    uint_32t cnt = 0;
    uint_32t b_pos = ctx->hdr_cnt & BLK_ADR_MASK;

    if (!hdr_len) {
        return RETURN_GOOD;
    }

    if (!((hdr - (UI8_PTR(ctx->hdr_cbc) + b_pos)) & BUF_ADRMASK)) {
        if(b_pos) {
            while(cnt < hdr_len && (b_pos & BUF_ADRMASK))
                UI8_PTR(ctx->hdr_cbc)[b_pos++] ^= hdr[cnt++];

            while(cnt + BUF_INC <= hdr_len && b_pos <= BLOCK_SIZE - BUF_INC)
            {
                *UINT_PTR(UI8_PTR(ctx->hdr_cbc) + b_pos) ^= *UINT_PTR(hdr + cnt);
                cnt += BUF_INC; b_pos += BUF_INC;
            }
        }

        while(cnt + BLOCK_SIZE <= hdr_len)
        {
            aes_encrypt(UI8_PTR(ctx->hdr_cbc), UI8_PTR(ctx->hdr_cbc), ctx->aes);
            xor_block_aligned(ctx->hdr_cbc, ctx->hdr_cbc, hdr + cnt);
            cnt += BLOCK_SIZE;
        }
    }
    else
    {
        if(b_pos)
            while(cnt < hdr_len && b_pos < BLOCK_SIZE)
                UI8_PTR(ctx->hdr_cbc)[b_pos++] ^= hdr[cnt++];

        while(cnt + BLOCK_SIZE <= hdr_len)
        {
            aes_encrypt(UI8_PTR(ctx->hdr_cbc), UI8_PTR(ctx->hdr_cbc), ctx->aes);
            xor_block(ctx->hdr_cbc, ctx->hdr_cbc, hdr + cnt);
            cnt += BLOCK_SIZE;
        }
    }

    while(cnt < hdr_len)
    {
        if(b_pos == BLOCK_SIZE || !b_pos)
        {
            aes_encrypt(UI8_PTR(ctx->hdr_cbc), UI8_PTR(ctx->hdr_cbc), ctx->aes);
            b_pos = 0;
        }
        UI8_PTR(ctx->hdr_cbc)[b_pos++] ^= hdr[cnt++];
    }

    ctx->hdr_cnt += cnt;
    return RETURN_GOOD;
*/
    return -1;
}



ret_type eax_auth_data(const io_t data[], unsigned long data_len, eax_ctx ctx[1])
{   uint_32t cnt = 0, b_pos = ctx->txt_acnt & BLK_ADR_MASK;

    if (!data_len) {
        return RETURN_GOOD;
    }

    if (!((data - (UI8_PTR(ctx->txt_cbc) + b_pos)) & BUF_ADRMASK)) {
        if(b_pos) {
            while (cnt < data_len && (b_pos & BUF_ADRMASK)) {
               UI8_PTR(ctx->txt_cbc)[b_pos++] ^= data[cnt++];
            }
            while(cnt + BUF_INC <= data_len && b_pos <= BLOCK_SIZE - BUF_INC) {
                *UINT_PTR(UI8_PTR(ctx->txt_cbc) + b_pos) ^= *UINT_PTR(data + cnt);
                cnt += BUF_INC; b_pos += BUF_INC;
            }
        }
        while(cnt + BLOCK_SIZE <= data_len) {
            aes_encrypt(UI8_PTR(ctx->txt_cbc), UI8_PTR(ctx->txt_cbc), ctx->aes);
            xor_block_aligned(ctx->txt_cbc, ctx->txt_cbc, data + cnt);
            cnt += BLOCK_SIZE;
        }
    }
    else {
        if(b_pos) {
            ///@todo Investigate Duffing this loop 
            while ((cnt < data_len) && (b_pos < BLOCK_SIZE)) {
               UI8_PTR(ctx->txt_cbc)[b_pos++] ^= data[cnt++];
            }
        }
        /// Don't need to optimize this because aes_encrypt() is the long-pole
        while ((cnt + BLOCK_SIZE) <= data_len) {
            aes_encrypt(UI8_PTR(ctx->txt_cbc), UI8_PTR(ctx->txt_cbc), ctx->aes);
            xor_block(ctx->txt_cbc, ctx->txt_cbc, data + cnt);
            cnt += BLOCK_SIZE;
        }
    }

    while (cnt < data_len) {
        if ((b_pos == BLOCK_SIZE) || !b_pos) {
            aes_encrypt(UI8_PTR(ctx->txt_cbc), UI8_PTR(ctx->txt_cbc), ctx->aes);
            b_pos = 0;
        }
        UI8_PTR(ctx->txt_cbc)[b_pos++] ^= data[cnt++];
    }

    ctx->txt_acnt += cnt;
    return RETURN_GOOD;
}




ret_type eax_crypt_data(io_t data[], unsigned long data_len, eax_ctx ctx[1])
{   
    uint_32t cnt = 0, b_pos = ctx->txt_ccnt & BLK_ADR_MASK;

    if (!data_len) {
        return RETURN_GOOD;
    }

    if(!((data - (UI8_PTR(ctx->enc_ctr) + b_pos)) & BUF_ADRMASK)) {
        if(b_pos) {
            while(cnt < data_len && (b_pos & BUF_ADRMASK)) {
                data[cnt++] ^= UI8_PTR(ctx->enc_ctr)[b_pos++];
              }

            while(cnt + BUF_INC <= data_len && b_pos <= BLOCK_SIZE - BUF_INC) {
                *UINT_PTR(data + cnt) ^= *UINT_PTR(UI8_PTR(ctx->enc_ctr) + b_pos);
                cnt += BUF_INC; b_pos += BUF_INC;
            }
        }

        while(cnt + BLOCK_SIZE <= data_len) {
            aes_encrypt(UI8_PTR(ctx->ctr_val), UI8_PTR(ctx->enc_ctr), ctx->aes);
            inc_ctr(ctx->ctr_val);
            xor_block_aligned(data + cnt, data + cnt, ctx->enc_ctr);
            cnt += BLOCK_SIZE;
        }
    }
    else {
        if (b_pos)
            while(cnt < data_len && b_pos < BLOCK_SIZE)
                data[cnt++] ^= UI8_PTR(ctx->enc_ctr)[b_pos++];

        while(cnt + BLOCK_SIZE <= data_len) {
            aes_encrypt(UI8_PTR(ctx->ctr_val), UI8_PTR(ctx->enc_ctr), ctx->aes);
            inc_ctr(ctx->ctr_val);
            xor_block(data + cnt, data + cnt, ctx->enc_ctr);
            cnt += BLOCK_SIZE;
        }
    }

    while(cnt < data_len) {
        if(b_pos == BLOCK_SIZE || !b_pos) {
            aes_encrypt(UI8_PTR(ctx->ctr_val), UI8_PTR(ctx->enc_ctr), ctx->aes);
            b_pos = 0;
            inc_ctr(ctx->ctr_val);
        }
        data[cnt++] ^= UI8_PTR(ctx->enc_ctr)[b_pos++];
    }

    ctx->txt_ccnt += cnt;
    return RETURN_GOOD;
}




ret_type eax_compute_tag(io_t tag[], eax_ctx ctx[1])
{   uint_32t i;
    uint_8t *p;

    if ((ctx->txt_acnt != ctx->txt_ccnt) && ctx->txt_ccnt > 0) {
        return RETURN_ERROR;
    }

    /* complete OMAC* for header value      */
    ///@todo does header need this at all, given that it is not used?
    p = UI8_PTR(ctx->pad_xvv);
    //if (i = ctx->hdr_cnt &  BLK_ADR_MASK) {
    //    UI8_PTR(ctx->hdr_cbc)[i] ^= 0x80;
    //    p += 16;
    //}

    ///@todo does header need this at all, given that it is not used?
    //xor_block_aligned(ctx->hdr_cbc, ctx->hdr_cbc, p);
    //aes_encrypt(UI8_PTR(ctx->hdr_cbc), UI8_PTR(ctx->hdr_cbc), ctx->aes);

    /* complete OMAC* for ciphertext value  */
    p = UI8_PTR(ctx->pad_xvv);
    i = ctx->txt_acnt &  BLK_ADR_MASK;
    if (i) {
    //if(i = ctx->txt_acnt &  BLK_ADR_MASK) {
        UI8_PTR(ctx->txt_cbc)[i] ^= 0x80;
        p += 16;
    }

    xor_block_aligned(ctx->txt_cbc, ctx->txt_cbc, p);
    aes_encrypt(UI8_PTR(ctx->txt_cbc), UI8_PTR(ctx->txt_cbc), ctx->aes);

    /* compute final authentication tag     */
    ///@todo experiment with word-based XOR
    //*(uint_32t*)tag = *(uint_32t*)ctx->nce_cbc \
                    ^ *(uint_32t*)ctx->txt_cbc \
                    ^ *(uint_32t*)ctx->hdr_cbc;
    //tag[0] = UI8_PTR(ctx->nce_cbc)[0] ^ UI8_PTR(ctx->txt_cbc)[0] ^ UI8_PTR(ctx->hdr_cbc)[0];
    //tag[1] = UI8_PTR(ctx->nce_cbc)[1] ^ UI8_PTR(ctx->txt_cbc)[1] ^ UI8_PTR(ctx->hdr_cbc)[1];
    //tag[2] = UI8_PTR(ctx->nce_cbc)[2] ^ UI8_PTR(ctx->txt_cbc)[2] ^ UI8_PTR(ctx->hdr_cbc)[2];
    //tag[3] = UI8_PTR(ctx->nce_cbc)[3] ^ UI8_PTR(ctx->txt_cbc)[3] ^ UI8_PTR(ctx->hdr_cbc)[3];
    tag[0] = UI8_PTR(ctx->nce_cbc)[0] ^ UI8_PTR(ctx->txt_cbc)[0];
    tag[1] = UI8_PTR(ctx->nce_cbc)[1] ^ UI8_PTR(ctx->txt_cbc)[1];
    tag[2] = UI8_PTR(ctx->nce_cbc)[2] ^ UI8_PTR(ctx->txt_cbc)[2];
    tag[3] = UI8_PTR(ctx->nce_cbc)[3] ^ UI8_PTR(ctx->txt_cbc)[3];
    
    ///@note changed to 0 - (ctx->txt_ccnt != ctx->txt_acnt)
    //return (ctx->txt_ccnt == ctx->txt_acnt) ? RETURN_GOOD : RETURN_WARN;
    return 0 - (ctx->txt_ccnt != ctx->txt_acnt);
}

ret_type eax_end(eax_ctx ctx[1])
{
    memset(ctx, 0, sizeof(eax_ctx));
    return RETURN_GOOD;
}

ret_type eax_encrypt(io_t data[], unsigned long data_len, eax_ctx ctx[1])
{
    eax_crypt_data(data, data_len, ctx);
    eax_auth_data(data, data_len, ctx);
    return RETURN_GOOD;
}

ret_type eax_decrypt(io_t data[], unsigned long data_len, eax_ctx ctx[1])
{
    eax_auth_data(data, data_len, ctx);
    eax_crypt_data(data, data_len, ctx);
    return RETURN_GOOD;
}







/** High-Level (User-Level) Encryption and Decryption routines for EAX
  * ========================================================================<BR>
  */

ret_type eax_encrypt_message(const io_t iv[], io_t msg[], unsigned long msg_len, eax_ctx ctx[1])
{
    ///@note [JPN] Tag is always dealt-with as the data right after the message
    io_t *tag = msg + msg_len;

    eax_init_message(iv, ctx);
    eax_encrypt(msg, msg_len, ctx);
    return eax_compute_tag(tag, ctx);
}



ret_type eax_decrypt_message(const io_t iv[], io_t msg[], unsigned long msg_len, eax_ctx ctx[1])
{   
    uint_8t     local_tag[4];
    ret_type    rr;

    ///@note [JPN] Tag is always dealt-with as the data right after the message
    uint_8t     tag[4];

    ///@todo Cortex-M supports non-aligned memory access.  I need to test it.
#   if defined (__UNALIGNED_ACCESS__)
        *((uint_32t*)tag) = *((uint_32t*)(msg + msg_len)); 
#   else
    {   io_t *cursor;
        cursor  = msg + msg_len;
        tag[0]  = *cursor++;
        tag[1]  = *cursor++;
        tag[2]  = *cursor++;
        tag[3]  = *cursor;
    }
#   endif
    
    // 1st pass decryption
    eax_init_message(iv, ctx);
    eax_decrypt(msg, msg_len, ctx);
    
    // 2nd pass tag computation and comparison
    rr  = eax_compute_tag(local_tag, ctx);
    rr -= ( *(uint_32t*)tag != *(uint_32t*)local_tag );
    return rr;
}




#if defined(__cplusplus)
    }
#endif
