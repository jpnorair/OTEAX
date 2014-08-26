About OTEAX
============
OTEAX is a streamlined implementation of AES-EAX cryptography, utilizing the OpenTag/DASH7 specification for AES-EAX, and intended for usage with ARM Cortex-M platforms.  OpenTag/DASH7 is intended for rather small frames (< 256 bytes) and it specifies AES-EAX usage with:
- 128 bit keys
- 56 bit (7 byte) Nonce
- 32 bit (4 byte) MAC field

OTEAX is currently in development -- it should be done sometime in September 2014.  Some information below is projective.

There are compile options that provide generic support of different AES-EAX specifications (different key length, nonce size, MAC size), however the hardware optimizations are not guaranteed for configurations other than the OpenTag/DASH7 spec.

Compilation may be done entirely with C, portable to any 32 bit machine, but compiler options exist to utilize certain ARM Cortex-M instructions which are available on all ARM Cortex-M devices (namely byte-swapping and bit-rotation).  Additionally, there is support for some cryptographic acceleration peripherals available on some STM32 devices.

OTEAX was adapted from Brian Gladman's AES library project.
[link]


Including
=========
Add the OTEAX files to your project, and use the following include declaration to allow linking to the top-level functions.  You don't need to include any other files.

#include <oteax.h>


Build Configuration
===================
The file oteax_config.h is the singular location for build configuration parameters.  You may modify the file to suit your needs, or you can pass the parameters as constants into your compiler's build string.  Refer to oteax_config.h for documentation.


Building & Linking
==================
There are several build options included with the generic makefile:
- all: builds "lib" and "test"
- clean: removes build files
- lib: builds liboteax.a
- test: builds oteax-test, a test app that runs in a POSIX shell environment

Typical usage is to build liboteax.a and it to your embedded project along with oteax.h, however you can also add the OTEAX code directly into your project if you choose.  OTEAX has no dependencies other than:
- stdint.h
- memcpy, memset, memcmp (often in string.h)
- ...




