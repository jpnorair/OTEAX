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
  * @file       /oteax/_testmain.c
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


#define cu8     const unsigned char
#define u8      unsigned char
#define ulong   unsigned long

static int test_encrypt(u8* nonce, u8* data, size_t datalen, u8* key);
static int test_decrypt(u8* nonce, u8* data, size_t datalen, u8* key);


static int __eaxcrypt(u8* nonce, u8* data, size_t datalen, u8* key,
                     int (*__crypt)(const void*, void*, unsigned long, eax_ctx*) )   {
    eax_ctx context;
    int     retval;
    
    retval = eax_init_and_key(key, &context);
    if (retval == 0) {
        retval = __crypt(nonce, data, datalen, &context);
        retval = retval ? -2 : 4;
    }
    return retval;
}


static int test_encrypt(u8* nonce, u8* data, size_t datalen, u8* key) {
    return __eaxcrypt(nonce, data, datalen, key, &eax_encrypt_message);
}


static int test_decrypt(u8* nonce, u8* data, size_t datalen, u8* key) {
    return __eaxcrypt(nonce, data, datalen, key, &eax_decrypt_message);
}



static void print_hex(u8* data, int length) {
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

    u8 data_buf[256];
    u8 nonce_buf[16];
    
    u8 test_key[16]    = { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };
    
    // Only the first 7/8 bytes are actually used, rest just RFU
    u8 test_nonce[16]  = { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };
    
    // Test payload: set to a prime-number of bytes to show non-aligned 
    // cipher feature of EAX
    u8 test_data[43]   = {   0,  1,  2,  3,  4,  5,  6,  7,  8,  9,
                            10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
                            20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
                            30, 31, 32, 33, 34, 35, 36, 37, 38, 39,
                            40, 41, 42
    };
    
    // Expected output of test encryption
    u8 test_check[47]  = {
        0xF2, 0xD7, 0xF3, 0xDE, 0xF3, 0x61, 0x85, 0x5F, 0xCB, 0x5E, 
        0x7E, 0xF4, 0x96, 0x51, 0x44, 0x6E, 0x24, 0x9C, 0x81, 0xA7, 
        0x63, 0x34, 0xF7, 0x4D, 0xC4, 0xD6, 0xAA, 0x31, 0xB3, 0x77, 
        0x65, 0x7B, 0x02, 0xB4, 0xF4, 0x49, 0xF1, 0x75, 0xF2, 0x1A, 
        0x54, 0x64, 0x50, 0xA1, 0x16, 0x0F, 0xE4 };
    
    // Clear buffers
    oteax_memset(data_buf, 0, sizeof(data_buf));
    oteax_memset(nonce_buf, 0, sizeof(nonce_buf));
    
    // Copy test_data to data_buf
    oteax_memcpy(data_buf, test_data, sizeof(test_data));
    
    // Run first encryption
    {   eax_ctx context;
        int     retval;

        retval = eax_init_and_key((cu8*)test_key, &context);
        if (retval != 0) {
            printf("eax_init_and_key() failed, unknown error\n\n");
        }
        else {
            for (i=0; i<44; i++) {
                printf("ks[%02d] = %u\n", i, context.aes[0].ks[i]);
            }
            printf("aes.inf.l = %u\n\n", context.aes[0].inf.l);

            retval = eax_encrypt_message((cu8*)test_nonce, (u8*)data_buf, sizeof(test_data), &context);
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
