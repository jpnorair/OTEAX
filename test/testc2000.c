/* Copyright 2014 JP Norair
  *
  * Licensed under the OpenTag License, Version 1.0 (the "License");
  * you may not use this file except in compliance with the License.
  * You may obtain a copy of the License at
  *
  * http://www.indigresso.com/wiki/doku.php?id=opentag:license_1_0
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  */
/**
  * @file       /oteax/testc2000.c
  * @author     JP Norair
  * @version    R100
  * @date       21 Aug 2014
  * @brief      OTEAX Test program
  *
  ******************************************************************************
  */



#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <oteax.h>


#define cuword  const uint32_t
#define uword   uint32_t
#define ulong   uword

static int test_encrypt(uword* nonce, uword* data, size_t datalen, uword* key);
static int test_decrypt(uword* nonce, uword* data, size_t datalen, uword* key);


static int __eaxcrypt(uword* nonce, uword* data, size_t datalen, uword* key,
                     int (*__crypt)(cuword*, uword*, ulong, eax_ctx*) )   {
    eax_ctx context;
    int     retval;
    
    retval = eax_init_and_key((cuword*)key, &context);
    if (retval == 0) {
        retval = __crypt((cuword*)nonce, (uword*)data, (ulong)datalen, &context);
        retval = retval ? -2 : 4;
    }
    return retval;
}


static int test_encrypt(uword* nonce, uword* data, size_t datalen, uword* key) {
    return __eaxcrypt(nonce, data, datalen, key, &eax_encrypt_message);
}


static int test_decrypt(uword* nonce, uword* data, size_t datalen, uword* key) {
    return __eaxcrypt(nonce, data, datalen, key, &eax_decrypt_message);
}



static void print_hex(uword* data, int length) {
    int i;
    for (i=0; i<length;) {
        printf("%02X ", data[i]);
        i++;
        if ((i % 16) == 0) {
            putchar('\n');
        }
    }
    if ((i % 16) != 0) {
        putchar('\n');
    }
}


int main(void) {
    int tag_size;
    int i;

    uword data_buf[64];
    uword nonce_buf[4];
    
    uword test_key[4]    = { 0x01020304, 0x04050607, 0x08090A0B, 0x0C0D0E0F };
    
    // Only the first 7 bytes are actually used, rest just RFU
    ///@todo may need to make NONCE 8 bytes for C2000
    uword test_nonce[4]  = { 0x01020304, 0x04050607, 0x08090A0B, 0x0C0D0E0F };
    
    // Test payload: set to a prime-number of bytes to show non-aligned 
    // cipher feature of EAX
    uword test_data[11]  = { 0x01020304, 0x05060708, 0x090A0B0C, 0x0D0E0F10,
                            0x11121314, 0x15161718, 0x191A1B1C, 0x1D1E1F20,
                            0x21222324, 0x25262728, 0x292A0000 };
    
    // Expected output of test encryption
    ///@todo get this output
    uword test_check[12]  = { };
    
    // Clear buffers
    oteax_memset(data_buf, 0, sizeof(data_buf));
    oteax_memset(nonce_buf, 0, sizeof(nonce_buf));
    
    // Copy test_data to data_buf
    oteax_memcpy(data_buf, test_data, sizeof(test_data));
    
    // Run first encryption
    {   eax_ctx context;
        int     retval;

        retval = eax_init_and_key((cuword*)test_key, &context);
        if (retval != 0) {
            printf("eax_init_and_key() failed, unknown error\n\n");
        }
        else {
            for (i=0; i<44; i++) {
                printf("ks[%02d] = %u\n", i, context.aes[0].ks[i]);
            }
            printf("aes.inf.l = %u\n", context.aes[0].inf.l);

            retval = eax_encrypt_message((cuword*)test_nonce, (uword*)data_buf, sizeof(test_data), &context);
            if (retval != 0) {
                printf("eax_encrypt_message() failed, unknown error\n\n");
            }
        }

        tag_size = 4;
    }

    //tag_size = test_encrypt(test_nonce, data_buf, sizeof(test_data), test_key);
    if (tag_size < 0) {
        printf("test_encrypt() failed, unknown error\n\n");
    }
    print_hex(data_buf, sizeof(test_data) + tag_size);
    putchar('\n');
    
    // Check against test
    {   int j;
        for (i=0, j=0; i<(sizeof(test_data) + tag_size); i++) {
            int k;
            k   = (data_buf[i] != test_check[i]);
            j  += k;
            
            if (j == 1) {
                printf("Errors: ");
            }
            if (k) {
                printf("%d, ", i);
            }
        }
        if (j != 0) {
            putchar('\n');
        }
        else {
            printf("Check done: no errors!\n");
        }
        putchar('\n');
    }
    
    // Decrypt
    tag_size = test_decrypt(test_nonce, data_buf, sizeof(test_data), test_key);
    if (tag_size != 4) {
        printf("test_decrypt() failed, unknown error\n\n");
    }
    print_hex(data_buf, sizeof(test_data));
    putchar('\n');

    return 0;
}
