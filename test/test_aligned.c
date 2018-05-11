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

#if defined(LITTLE_ENDIAN)
#   define MAKE_U8(W)   ((((uint32_t)(W)>>24)&0xFF)|(((uint32_t)(W)>>8)&0xFF00)|(((uint32_t)(W)<<8)&0xFF0000)|(((uint32_t)(W)<<24)&0xFF000000))
#else
#   define MAKE_U8(W)   (W)
#endif


static int test_encrypt(uword* nonce, uword* data, size_t datalen, uword* key);
static int test_decrypt(uword* nonce, uword* data, size_t datalen, uword* key);


static int __eaxcrypt(uword* nonce, uword* data, size_t datalen, uword* key,
                     int (*__crypt)(const void*, void*, unsigned long, eax_ctx*) )   {
    eax_ctx context;
    int     retval;
    
    retval = eax_init_and_key(key, &context);
    if (retval == 0) {
        retval = __crypt((const void*)nonce, (void*)data, (ulong)datalen, &context);
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



static void print_hex(void* data, int length) {
    int i;
    uint8_t* data8 = data;
    
    for (i=0; i<length;) {
        printf("%02X ", data8[i]);
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
    
    uword test_key[4]    = { MAKE_U8(0x00010203), 
                             MAKE_U8(0x04050607), 
                             MAKE_U8(0x08090A0B), 
                             MAKE_U8(0x0C0D0E0F) 
    };
    
    // Only the first 7 bytes are actually used, rest just RFU
    ///@todo may need to make NONCE 8 bytes for C2000
    uword test_nonce[4]  = { MAKE_U8(0x00010203), 
                             MAKE_U8(0x04050607), 
                             MAKE_U8(0x08090A0B), 
                             MAKE_U8(0x0C0D0E0F) 
    };
    
    // Test payload: set to a prime-number of bytes to show non-aligned 
    // cipher feature of EAX
    uword test_data[11]  = { MAKE_U8(0x00010203), 
                             MAKE_U8(0x04050607), 
                             MAKE_U8(0x08090A0B), 
                             MAKE_U8(0x0C0D0E0F),
                             MAKE_U8(0x10111213), 
                             MAKE_U8(0x14151617), 
                             MAKE_U8(0x18191A1B), 
                             MAKE_U8(0x1C1D1E1F),
                             MAKE_U8(0x20212223), 
                             MAKE_U8(0x24252627), 
                             MAKE_U8(0x28292A00) 
    };
    
    // Expected output of test encryption
    ///@todo get this output
    uword test_check[12] = { MAKE_U8(0xF2D7F3DE), 
                             MAKE_U8(0xF361855F), 
                             MAKE_U8(0xCB5E7EF4), 
                             MAKE_U8(0x9651446E),
                             MAKE_U8(0x249C81A7), 
                             MAKE_U8(0x6334F74D), 
                             MAKE_U8(0xC4D6AA31), 
                             MAKE_U8(0xB377657B),
                             MAKE_U8(0x02B4F449), 
                             MAKE_U8(0xF175F21A), 
                             MAKE_U8(0x546450DC), 
                             MAKE_U8(0x38B8B2C7)  };
    
    // Clear buffers
    memset(data_buf, 0, sizeof(data_buf));
    memset(nonce_buf, 0, sizeof(nonce_buf));
    
    // Copy test_data to data_buf
    memcpy(data_buf, test_data, sizeof(test_data));
    
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
    
    // Print the input, for cross-checking, and then the output
    printf("Plain-text Input\n");
    print_hex(test_data, sizeof(test_data));
    printf("\nOutput Data from EAX:\n");
    print_hex(data_buf, sizeof(test_data) + tag_size);
    putchar('\n');
    
    // Check against test
    {   int j;
        int len_words = (sizeof(test_data) + tag_size) / 4;
        for (i=0, j=0; i<len_words; i++) {
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
            printf("\nOutput should be:\n");
            print_hex(test_check, sizeof(test_check));
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
    printf("Decrypted Output\n");
    print_hex(data_buf, sizeof(test_data));
    putchar('\n');

    return 0;
}
