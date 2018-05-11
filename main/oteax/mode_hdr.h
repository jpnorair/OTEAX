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
Issue Date: 07/10/2010

This header file is an INTERNAL file which supports mode implementation
*/

#ifndef _MODE_HDR_H
#define _MODE_HDR_H

#include <limits.h>
#include "brg_endian.h"


///@note this is to link against platform-specific memcpy functions.
#if (defined(__OPENTAG__) || defined(__opentag__) || defined(__LIBOTFS__))
#   include <platform/config.h>
#   include <otlib/memcpy.h>

///@todo [JP Norair 25-Aug-2014] THis is a hack for STM32L CM3 on OpenTag
#   if (defined(__STM32__) || defined(_STM32x_))
//#       include <cm3_endian.h>
//#       include <cm3_byteswap.h>
//#       include <cm3_bitrotate.h>

#       define rotl32       __rotl32
#       define rotr32       __rotr32
#       define rotl16       __rotr
#       define bswap_16(x)  __bswap16(x)
#       define bswap_32(x)  __bswap32(x)
#       define bswap_64(x)  __bswap64(x)

#       define NET_ENDIAN32(x)  bswap_32(x)

#   endif

#else
#   include <string.h>
#endif



#if defined(__C2000__)
    ///@todo replace with Some assembly hooks for fast byte swap functions, if they exist
#   define NET_ENDIAN32(x)  ((((uint32_t)(W)>>24)&0xFF)|(((uint32_t)(W)>>8)&0xFF00)|(((uint32_t)(W)<<8)&0xFF0000)|(((uint32_t)(W)<<24)&0xFF000000))

#elif (PLATFORM_BYTE_ORDER == IS_LITTLE_ENDIAN)
    ///@todo potentially include byteswap.h or similar
//#   include <byteswap.h>
#   define NET_ENDIAN32(x)  ((((uint32_t)(W)>>24)&0xFF)|(((uint32_t)(W)>>8)&0xFF00)|(((uint32_t)(W)<<8)&0xFF0000)|(((uint32_t)(W)<<24)&0xFF000000))

#else 
#   define NET_ENDIAN32(x)  (x)
#endif



/*  This define sets the units in which buffers are processed.  This code
    can provide significant speed gains if buffers can be processed in
    32 or 64 bit chunks rather than in bytes.  This define sets the units
    in which buffers will be accessed if possible
*/

///@note hack to force C2000 to use 32 bit data elements
#if defined(__C2000__)
#   undef UINT_BITS
#   define UINT_BITS    32
#endif

#if !defined( UINT_BITS )
#   if (PLATFORM_BYTE_ORDER == IS_BIG_ENDIAN)
#       if 0
#           define UINT_BITS  32
#       elif 1
#           define UINT_BITS  64
#       endif
#   elif defined( _WIN64 )
#       define UINT_BITS 64
#   else
#       define UINT_BITS 32
#   endif
#endif

#if UINT_BITS == 64 && !defined( NEED_UINT_64T )
#  define NEED_UINT_64T
#endif

#include "brg_types.h"

/*  Use of inlines is preferred but code blocks can also be expanded inline
    using 'defines'.  But the latter approach will typically generate a LOT
    of code and is not recommended. 
*/
#if 1 && !defined( USE_INLINING )
#   define USE_INLINING
#endif

#if defined( _MSC_VER )
#   if _MSC_VER >= 1400
#       include <stdlib.h>
#       include <intrin.h>
#       pragma intrinsic(memset)
#       pragma intrinsic(memcpy)
#       define rotl32        _rotl
#       define rotr32        _rotr
#       define rotl64        _rotl64
#       define rotr64        _rotl64
#       define bswap_16(x)   _byteswap_ushort(x)
#       define bswap_32(x)   _byteswap_ulong(x)
#       define bswap_64(x)   _byteswap_uint64(x)
#   else
#       define rotl32 _lrotl
#       define rotr32 _lrotr
#   endif
#endif

#if defined( USE_INLINING )
#   if defined( _MSC_VER )
#       define mh_decl __inline
#   elif defined( __GNUC__ ) || defined( __GNU_LIBRARY__ )
#       define mh_decl static inline
#   else
#       define mh_decl static
#   endif
#endif

#if defined(__cplusplus)
    extern "C" {
#endif


#define  UI8_PTR(x)     UPTR_CAST(x,  8)
#define UI16_PTR(x)     UPTR_CAST(x, 16)
#define UI32_PTR(x)     UPTR_CAST(x, 32)
#define UI64_PTR(x)     UPTR_CAST(x, 64)
#define UINT_PTR(x)     UPTR_CAST(x, UINT_BITS)
#if defined(__ALIGN32__)
#   define IO_PTR(x)    UPTR_CAST(x, 32)
#else
#   define IO_PTR(x)    UPTR_CAST(x, 8)
#endif

#define  UI8_VAL(x)     UINT_CAST(x,  8)
#define UI16_VAL(x)     UINT_CAST(x, 16)
#define UI32_VAL(x)     UINT_CAST(x, 32)
#define UI64_VAL(x)     UINT_CAST(x, 64)
#define UINT_VAL(x)     UINT_CAST(x, UINT_BITS)

#define BUF_INC          (UINT_BITS >> 3)
#define BUF_ADRMASK     ((UINT_BITS >> 3) - 1)

#define rep2_u2(f,r,x)    f( 0,r,x); f( 1,r,x) 
#define rep2_u4(f,r,x)    f( 0,r,x); f( 1,r,x); f( 2,r,x); f( 3,r,x) 
#define rep2_u16(f,r,x)   f( 0,r,x); f( 1,r,x); f( 2,r,x); f( 3,r,x); \
                          f( 4,r,x); f( 5,r,x); f( 6,r,x); f( 7,r,x); \
                          f( 8,r,x); f( 9,r,x); f(10,r,x); f(11,r,x); \
                          f(12,r,x); f(13,r,x); f(14,r,x); f(15,r,x)

#define rep2_d2(f,r,x)    f( 1,r,x); f( 0,r,x) 
#define rep2_d4(f,r,x)    f( 3,r,x); f( 2,r,x); f( 1,r,x); f( 0,r,x) 
#define rep2_d16(f,r,x)   f(15,r,x); f(14,r,x); f(13,r,x); f(12,r,x); \
                          f(11,r,x); f(10,r,x); f( 9,r,x); f( 8,r,x); \
                          f( 7,r,x); f( 6,r,x); f( 5,r,x); f( 4,r,x); \
                          f( 3,r,x); f( 2,r,x); f( 1,r,x); f( 0,r,x)

#define rep3_u2(f,r,x,y,c)  f( 0,r,x,y,c); f( 1,r,x,y,c) 
#define rep3_u4(f,r,x,y,c)  f( 0,r,x,y,c); f( 1,r,x,y,c); f( 2,r,x,y,c); f( 3,r,x,y,c) 
#define rep3_u16(f,r,x,y,c) f( 0,r,x,y,c); f( 1,r,x,y,c); f( 2,r,x,y,c); f( 3,r,x,y,c); \
                            f( 4,r,x,y,c); f( 5,r,x,y,c); f( 6,r,x,y,c); f( 7,r,x,y,c); \
                            f( 8,r,x,y,c); f( 9,r,x,y,c); f(10,r,x,y,c); f(11,r,x,y,c); \
                            f(12,r,x,y,c); f(13,r,x,y,c); f(14,r,x,y,c); f(15,r,x,y,c)

#define rep3_d2(f,r,x,y,c)  f( 1,r,x,y,c); f( 0,r,x,y,c) 
#define rep3_d4(f,r,x,y,c)  f( 3,r,x,y,c); f( 2,r,x,y,c); f( 1,r,x,y,c); f( 0,r,x,y,c) 
#define rep3_d16(f,r,x,y,c) f(15,r,x,y,c); f(14,r,x,y,c); f(13,r,x,y,c); f(12,r,x,y,c); \
                            f(11,r,x,y,c); f(10,r,x,y,c); f( 9,r,x,y,c); f( 8,r,x,y,c); \
                            f( 7,r,x,y,c); f( 6,r,x,y,c); f( 5,r,x,y,c); f( 4,r,x,y,c); \
                            f( 3,r,x,y,c); f( 2,r,x,y,c); f( 1,r,x,y,c); f( 0,r,x,y,c)

/* function pointers might be used for fast XOR operations */

typedef void (*xor_function)(void* r, const void* p, const void* q);




/* left and right rotates on 32 and 64 bit variables */
///@todo put these in ARM inline assembly

#if !defined( rotl32 )  /* NOTE: 0 <= n <= 32 ASSUMED */
mh_decl uint_32t rotl32(uint_32t x, int n) {
    return (((x) << n) | ((x) >> (32 - n)));
}
#endif


#if !defined( rotr32 )  /* NOTE: 0 <= n <= 32 ASSUMED */
mh_decl uint_32t rotr32(uint_32t x, int n) {
    return (((x) >> n) | ((x) << (32 - n)));
}
#endif


#if UINT_BITS == 64 && !defined( rotl64 )  /* NOTE: 0 <= n <= 64 ASSUMED */
mh_decl uint_64t rotl64(uint_64t x, int n) {
    return (((x) << n) | ((x) >> (64 - n)));
}
#endif


#if UINT_BITS == 64 && !defined( rotr64 )  /* NOTE: 0 <= n <= 64 ASSUMED */
mh_decl uint_64t rotr64(uint_64t x, int n) {
    return (((x) >> n) | ((x) << (64 - n)));
}
#endif




/* byte order inversions for 16, 32 and 64 bit variables */
///@todo put these in ARM inline assembly

#if !defined(bswap_16)
mh_decl uint_16t bswap_16(uint_16t x) {
    return (uint_16t)((x >> 8) | (x << 8));
}
#endif


#if !defined(bswap_32)
//#error "This should not be compiled with OpenTag"
mh_decl uint_32t bswap_32(uint_32t x) {
    return ((rotr32((x), 24) & 0x00ff00ff) | (rotr32((x), 8) & 0xff00ff00));
}
#endif


#if UINT_BITS == 64 && !defined(bswap_64)
mh_decl uint_64t bswap_64(uint_64t x) {   
    return bswap_32((uint_32t)(x >> 32)) | ((uint_64t)bswap_32((uint_32t)x) << 32);
}
#endif



/* support for fast aligned buffer move, xor and byte swap operations - 
   source and destination buffers for move and xor operations must not 
   overlap, those for byte order revesal must either not overlap or
   must be identical
*/

#define f_copy(n,p,q)     p[n] = q[n]
#define f_xor(n,r,p,q,c)  r[n] = c(p[n] ^ q[n])



mh_decl void copy_block(void* p, const void* q) {
    memcpy(p, q, 16);
}



mh_decl void copy_block_aligned(void *p, const void *q) {
#if UINT_BITS == 8
    memcpy(p, q, 16);
#elif UINT_BITS == 32
    rep2_u4(f_copy,UINT_PTR(p),UINT_PTR(q));
#else
    rep2_u2(f_copy,UINT_PTR(p),UINT_PTR(q));
#endif
}




mh_decl void xor_block(void *r, const void* p, const void* q) {
    rep3_u16(f_xor, UI8_PTR(r), UI8_PTR(p), UI8_PTR(q), UI8_VAL);
}




mh_decl void xor_block_aligned(void *r, const void *p, const void *q) {
#if UINT_BITS == 8
    rep3_u16(f_xor, UINT_PTR(r), UINT_PTR(p), UINT_PTR(q), UINT_VAL);
#elif UINT_BITS == 32
    rep3_u4(f_xor, UINT_PTR(r), UINT_PTR(p), UINT_PTR(q), UINT_VAL);
#else
    rep3_u2(f_xor, UINT_PTR(r), UINT_PTR(p), UINT_PTR(q), UINT_VAL);
#endif
}




/* byte swap within 32-bit words in a 16 byte block; don't move 32-bit words */
mh_decl void bswap32_block(void *d, const void* s) {
#if UINT_BITS == 8
    uint_8t t;
    t = UINT_PTR(s)[ 0]; UINT_PTR(d)[ 0] = UINT_PTR(s)[ 3]; UINT_PTR(d)[ 3] = t;
    t = UINT_PTR(s)[ 1]; UINT_PTR(d)[ 1] = UINT_PTR(s)[ 2]; UINT_PTR(d)[ 2] = t;
    t = UINT_PTR(s)[ 4]; UINT_PTR(d)[ 4] = UINT_PTR(s)[ 7]; UINT_PTR(d)[ 7] = t;
    t = UINT_PTR(s)[ 5]; UINT_PTR(d)[ 5] = UINT_PTR(s)[ 6]; UINT_PTR(d) [6] = t;
    t = UINT_PTR(s)[ 8]; UINT_PTR(d)[ 8] = UINT_PTR(s)[11]; UINT_PTR(d)[12] = t;
    t = UINT_PTR(s)[ 9]; UINT_PTR(d)[ 9] = UINT_PTR(s)[10]; UINT_PTR(d)[10] = t;
    t = UINT_PTR(s)[12]; UINT_PTR(d)[12] = UINT_PTR(s)[15]; UINT_PTR(d)[15] = t;
    t = UINT_PTR(s)[13]; UINT_PTR(d)[ 3] = UINT_PTR(s)[14]; UINT_PTR(d)[14] = t;
#elif UINT_BITS == 32
    UINT_PTR(d)[0] = bswap_32(UINT_PTR(s)[0]); UINT_PTR(d)[1] = bswap_32(UINT_PTR(s)[1]);
    UINT_PTR(d)[2] = bswap_32(UINT_PTR(s)[2]); UINT_PTR(d)[3] = bswap_32(UINT_PTR(s)[3]);
#else
    UI32_PTR(d)[0] = bswap_32(UI32_PTR(s)[0]); UI32_PTR(d)[1] = bswap_32(UI32_PTR(s)[1]);
    UI32_PTR(d)[2] = bswap_32(UI32_PTR(s)[2]); UI32_PTR(d)[3] = bswap_32(UI32_PTR(s)[3]);
#endif
}




/* byte swap within 64-bit words in a 16 byte block; don't move 64-bit words */
mh_decl void bswap64_block(void *d, const void* s) {
#if UINT_BITS == 8
    uint_8t t;
    t = UINT_PTR(s)[ 0]; UINT_PTR(d)[ 0] = UINT_PTR(s)[ 7]; UINT_PTR(d)[ 7] = t;
    t = UINT_PTR(s)[ 1]; UINT_PTR(d)[ 1] = UINT_PTR(s)[ 6]; UINT_PTR(d)[ 6] = t;
    t = UINT_PTR(s)[ 2]; UINT_PTR(d)[ 2] = UINT_PTR(s)[ 5]; UINT_PTR(d)[ 5] = t;
    t = UINT_PTR(s)[ 3]; UINT_PTR(d)[ 3] = UINT_PTR(s)[ 3]; UINT_PTR(d) [3] = t;
    t = UINT_PTR(s)[ 8]; UINT_PTR(d)[ 8] = UINT_PTR(s)[15]; UINT_PTR(d)[15] = t;
    t = UINT_PTR(s)[ 9]; UINT_PTR(d)[ 9] = UINT_PTR(s)[14]; UINT_PTR(d)[14] = t;
    t = UINT_PTR(s)[10]; UINT_PTR(d)[10] = UINT_PTR(s)[13]; UINT_PTR(d)[13] = t;
    t = UINT_PTR(s)[11]; UINT_PTR(d)[11] = UINT_PTR(s)[12]; UINT_PTR(d)[12] = t;
#elif UINT_BITS == 32
    uint_32t t;
    t = bswap_32(UINT_PTR(s)[0]); UINT_PTR(d)[0] = bswap_32(UINT_PTR(s)[1]); UINT_PTR(d)[1] = t;
    t = bswap_32(UINT_PTR(s)[2]); UINT_PTR(d)[2] = bswap_32(UINT_PTR(s)[2]); UINT_PTR(d)[3] = t;
#else
    UINT_PTR(d)[0] = bswap_64(UINT_PTR(s)[0]);  UINT_PTR(d)[1] = bswap_64(UINT_PTR(s)[1]); 
#endif
}




mh_decl void bswap128_block(void *d, const void* s) {
#if UINT_BITS == 8
    uint_8t t;
    t = UINT_PTR(s)[0]; UINT_PTR(d)[0] = UINT_PTR(s)[15]; UINT_PTR(d)[15] = t;
    t = UINT_PTR(s)[1]; UINT_PTR(d)[1] = UINT_PTR(s)[14]; UINT_PTR(d)[14] = t;
    t = UINT_PTR(s)[2]; UINT_PTR(d)[2] = UINT_PTR(s)[13]; UINT_PTR(d)[13] = t;
    t = UINT_PTR(s)[3]; UINT_PTR(d)[3] = UINT_PTR(s)[12]; UINT_PTR(d)[12] = t;
    t = UINT_PTR(s)[4]; UINT_PTR(d)[4] = UINT_PTR(s)[11]; UINT_PTR(d)[11] = t;
    t = UINT_PTR(s)[5]; UINT_PTR(d)[5] = UINT_PTR(s)[10]; UINT_PTR(d)[10] = t;
    t = UINT_PTR(s)[6]; UINT_PTR(d)[6] = UINT_PTR(s)[ 9]; UINT_PTR(d)[ 9] = t;
    t = UINT_PTR(s)[7]; UINT_PTR(d)[7] = UINT_PTR(s)[ 8]; UINT_PTR(d)[ 8] = t;
#elif UINT_BITS == 32
    uint_32t t;
    t = bswap_32(UINT_PTR(s)[0]); UINT_PTR(d)[0] = bswap_32(UINT_PTR(s)[3]); UINT_PTR(d)[3] = t;
    t = bswap_32(UINT_PTR(s)[1]); UINT_PTR(d)[1] = bswap_32(UINT_PTR(s)[2]); UINT_PTR(d)[2] = t;
#else
    uint_64t t;
    t = bswap_64(UINT_PTR(s)[0]); UINT_PTR(d)[0] = bswap_64(UINT_PTR(s)[1]); UINT_PTR(d)[1] = t;
#endif
}




/* platform byte order to big or little endian order for 16, 32 and 64 bit variables */

#if PLATFORM_BYTE_ORDER == IS_BIG_ENDIAN
#   define uint_16t_to_le(x) (x) = bswap_16((x))
#   define uint_32t_to_le(x) (x) = bswap_32((x))
#   define uint_64t_to_le(x) (x) = bswap_64((x))
#   define uint_16t_to_be(x)
#   define uint_32t_to_be(x)
#   define uint_64t_to_be(x)

#else
#   define uint_16t_to_le(x)
#   define uint_32t_to_le(x)
#   define uint_64t_to_le(x)
#   define uint_16t_to_be(x) (x) = bswap_16((x))
#   define uint_32t_to_be(x) (x) = bswap_32((x))
#   define uint_64t_to_be(x) (x) = bswap_64((x))

#endif




#if defined(__cplusplus)
}
#endif

#endif
