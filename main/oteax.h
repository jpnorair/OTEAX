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
/** @brief EAX for OpenTag
  * Adapted for OpenTag by JP Norair, written by Brian Gladman
  * 
  * To include the EAX encryption system into your OpenTag project, make sure
  * to compile all the C files in this directory, and make sure to include this
  * file.  You don't need to include any other files.
  *
  * Here is a list of the functions you need to call in the OT driver (which
  * acts as little more than a wrapper).  The return value is 0 when everything
  * works, else -1.
  * <LI> eax_init_and_key() : Initialize EAX engine </LI>
  * <LI> eax_end() : De-initialize ("free") EAX engine </LI>
  * <LI> eax_encrypt_message() : Encrypts a message in place </LI>
  * <LI> eax_decrypt_message() : Decrypts a message in place </LI>
  * 
  * Each of these functions include an argument "ctx" of type "eax_ctx" which
  * must be supplied and retained by the driver through the cryptography 
  * process.  The AES-EAX driver in OpenTag will alias this as EAX_t.
  * 
  * @note [JP Norair, 25 Aug 2014]
  * This version of eax.h is streamlined for OpenTag/Mode2.  It uses settings
  * and configurations that are in some places hard-coded for optimization, to
  * do only the features required (and no more)
  */

#ifndef _EAX_H
#define _EAX_H

/*  This define sets the memory alignment that will be used for fast move
    and xor operations on buffers when the alignment matches this value. 
*/
///@note [JPN] I changed the default here from 64 bits to 32 bits
#if !defined( UINT_BITS )
#   if 0
#       define UINT_BITS 64
#   elif 1
#       define UINT_BITS 32
#   else
#       define UINT_BITS  8
#   endif
#endif

#if UINT_BITS == 64 && !defined( NEED_UINT_64T )
#   define NEED_UINT_64T
#endif

#include "oteax/aes.h"

#if defined(__cplusplus)
extern "C"
{
#endif

/*  After encryption or decryption operations the return value of
    'compute tag' will be one of the values RETURN_GOOD, RETURN_WARN
    or RETURN_ERROR, the latter indicating an error. A return value
    RETURN_GOOD indicates that both encryption and authentication
    have taken place and resulted in the returned tag value. If
    the returned value is RETURN_WARN, the tag value is the result
    of authentication alone without encryption (CCM) or decryption
    (GCM and EAX).
*/
#ifndef RETURN_GOOD
#   define RETURN_WARN      1
#   define RETURN_GOOD      0
#   define RETURN_ERROR    -1
#endif

#ifndef RET_TYPE_DEFINED
  typedef int  ret_type;
#endif
UINT_TYPEDEF(eax_unit_t, UINT_BITS);
BUFR_TYPEDEF(eax_buf_t, UINT_BITS, AES_BLOCK_SIZE);
BUFR_TYPEDEF(eax_dbuf_t, UINT_BITS, 2 * AES_BLOCK_SIZE);

#define EAX_BLOCK_SIZE  AES_BLOCK_SIZE

/* The EAX-AES  context  */

typedef struct {
    eax_buf_t       ctr_val;               /* CTR counter value            */
    eax_buf_t       enc_ctr;               /* encrypted CTR block          */
    //eax_buf_t       hdr_cbc;               /* encrypt(1), for header CBC   */
    eax_buf_t       txt_cbc;               /* encrypt(2), for ctext CBC    */
    eax_buf_t       nce_cbc;               /* encrypt (0|nonce), for iv CBC*/
    eax_dbuf_t      pad_xvv;               /* {02} encrypt(0), pad values  */
    aes_encrypt_ctx aes[1];                 /* AES encryption context       */
    //uint_32t        hdr_cnt;                /* header bytes so far          */
    uint_32t        txt_ccnt;               /* text bytes so far (encrypt)  */
    uint_32t        txt_acnt;               /* text bytes so far (auth)     */
} eax_ctx;



/* The following calls handle mode initialisation, keying and completion    */

/** @brief First step for EAX encryption or decryption: initialize context and key
  * @param key  (const void*) AES key.  Typically 128 bits.
  * @param ctx  (eax_ctx) Mode context, which acts as the control input.
  * @retval     (ret_type) returns 0 on success.
  *
  * The Key input is of type (const void*).  This is usually uint8_t*, but it
  * can be set to larger increments in order to improve performance or for
  * compatibility on machines without byte addressing (some DSPs).
  */
ret_type eax_init_and_key(const void* key, eax_ctx ctx[1]);


/** @brief Wrap-up cryptography process, do context clean-up.
  * @param ctx  (eax_ctx) Mode context, which acts as the control input.
  * @retval     (ret_type) returns 0 on success.
  */
ret_type eax_end(eax_ctx ctx[1]);




/* The following calls handle complete messages in memory as one operation  */

/** @brief Single-call function to encrypt a plain-text data input.
  * @param iv       (const void*) Initialization vector.
  * @param msg      (void*) Plain Text message data
  * @param msg_len  (unsigned long) Number of bytes in length, for msg
  * @param ctx      (eax_ctx) Mode context, which acts as the control input.
  * @retval         (ret_type) returns 0 on success.
  */
ret_type eax_encrypt_message(const void* iv, void* msg, unsigned long msg_len, eax_ctx ctx[1]);


/** @brief Single-call function to decrypt an EAX data input into plain-text.
  * @param iv       (const void*) Initialization vector.
  * @param msg      (void*) Plain Text message data
  * @param msg_len  (unsigned long) Number of bytes in length, for msg
  * @param ctx      (eax_ctx) Mode context, which acts as the control input.
  * @retval         (ret_type) returns 0 (success) when authentication tag 
  *                 of msg matches the computed tag.
  */
ret_type eax_decrypt_message(const void* iv, void* msg, unsigned long msg_len, eax_ctx ctx[1]);





/* The following calls handle messages in a sequence of operations followed */
/* by tag computation after the sequence has been completed. In these calls */
/* the user is responsible for verfiying the computed tag on decryption     */

/** @brief Initialize EAX message with IV.
  * @param iv       (const io_t*) Initialization vector.
  * @param ctx      (eax_ctx) Mode context, which acts as the control input.
  * @retval         (ret_type) returns 0 on success.
  */
ret_type eax_init_message(const io_t* iv, eax_ctx ctx[1]);


/** @brief Encrypt data using already initialized context, IV, Header
  * @param data     (io_t*) In-place data input/output
  * @param data_len (unsigned long) Length of data in io_t units.
  * @param ctx      (eax_ctx) Mode context, which acts as the control input.
  * @retval         (ret_type) returns 0 on success.
  */
ret_type eax_encrypt(io_t* data, unsigned long data_len, eax_ctx ctx[1]);


/** @brief Decrypt data using already initialized context, IV, Header
  * @param data     (void*) In-place data input/output
  * @param data_len (unsigned long) Length of data in io_t units.
  * @param ctx      (eax_ctx) Mode context, which acts as the control input.
  * @retval         (ret_type) returns 0 on success.
  */
ret_type eax_decrypt(io_t* data, unsigned long data_len, eax_ctx ctx[1]);


/** @brief Compute tag using already initialized context, IV, Header, Data
  * @param tag      (io_t*) In-place tag input/output
  * @param ctx      (eax_ctx) Mode context, which acts as the control input.
  * @retval         (ret_type) returns 0 on success.
  */
ret_type eax_compute_tag(io_t* tag, eax_ctx ctx[1]);




/*  The use of the following calls should be avoided if possible because 
    their use requires a very good understanding of the way this encryption 
    mode works and the way in which this code implements it in order to use 
    them correctly.

    The eax_auth_data routine is used to authenticate encrypted message data.
    In message encryption eax_crypt_data must be called before eax_auth_data
    is called since it is encrypted data that is authenticated.  In message
    decryption authentication must occur before decryption and data can be
    authenticated without being decrypted if necessary.

    If these calls are used it is up to the user to ensure that these routines
    are called in the correct order and that the correct data is passed to 
    them.

    When eax_compute_tag is called it is assumed that an error in use has
    occurred if both encryption (or decryption) and authentication have taken
    place but the total lengths of the message data respectively authenticated
    and encrypted are not the same. If authentication has taken place but 
    there has been no corresponding encryption or decryption operations (none
    at all) only a warning is issued. This should be treated as an error if it 
    occurs during encryption but it is only signalled as a warning as it might 
    be intentional when decryption operations are involved (this avoids having
    different compute tag functions for encryption and decryption). Decryption
    operations can be undertaken freely after authetication but if the tag is
    computed after such operations an error will be signalled if the lengths
    of the data authenticated and decrypted don't match.
*/



/** @brief Run EAX Authenticator over initialized context, with input data.
  * @param data     (const io_t*) Data input
  * @param data_len (unsigned long) Length of data in io_t units.
  * @param ctx      (eax_ctx) Mode context, which acts as the control input.
  * @retval         (ret_type) returns 0 on success.
  */
ret_type eax_auth_data(const io_t* data, unsigned long data_len, eax_ctx ctx[1]);


/** @brief Run EAX Cryptographer over initialized context, with input data.
  * @param data     (void*) In place Data input/output
  * @param data_len (unsigned long) Length of data in io_t units.
  * @param ctx      (eax_ctx) Mode context, which acts as the control input.
  * @retval         (ret_type) returns 0 on success.
  *
  * @note EAX is a symmetric encryption, so encrypt and decrypt is the same
  * actual process.
  */
ret_type eax_crypt_data(io_t* data, unsigned long data_len, eax_ctx ctx[1]);

#if defined(__cplusplus)
}
#endif

#endif
