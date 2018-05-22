# About OTEAX

OTEAX is a streamlined implementation of AES-EAX cryptography, utilizing the OpenTag/DASH7 specification for AES-EAX, and intended for usage with ARM Cortex-M platforms.  OpenTag/DASH7 is intended for rather small frames (< 256 bytes) and it specifies AES-EAX usage with:

* 128 bit keys
* 56 bit (7 byte) Nonce
* 32 bit (4 byte) MAC field

## Why Use EAX?
EAX is the perfect AES cipher mode for usage with IoT microcontrollers.

1. It's completely royalty-free and the license is permissive.
2. It doesn't require any block padding bytes.  For example, the commonly used CBC and CMAC ciphers enforce 16 byte alignment in cipher-data.  EAX does not.
3. EAX has symmetric encoding and decoding stages, so the amount of memory and program code required for the application is comparably small.  A build for Cortex-M3 can be as small as 16KB, and requires less than 256 bytes of memory.  If you decide to save contexts on the heap, the size of the stack memory goes down to less than 64 bytes.
4. EAX is slower than OFB and GCM cipher modes, but it is faster than CBC or CMAC.  It requires much less RAM and program code than all.
5. Additional hardware optimization is possible via co-processors that support AES-CTR and AES-CBC(many).  CTR and CBC are part of the inner loop in EAX.

## What is Novel About OTEAX?

OTEAX was adapted from [Brian Gladman's AES library](https://github.com/BrianGladman/aes). All of the parts of Brian Gladman's code that were not specifically required to implement OTEAX were removed.

1. OTEAX is entirely in C, making it portable for lots of embedded applications.  It is more accessible, as well, for engineers wishing to make modifications.  If simply you wish to build an AES library on desktop, use Brian Gladman's. 
2. Due in part to the symmetric nature of EAX, OTEAX has a ridiculously simple API.  It's very easy to integrate into other projects.
3. It is expected to be used mainly with Cortex-M devices and other microcontrollers.  
4. OTEAX has several build options to simplify optimization
5. OTEAX has a build option to support 32 bit aligned data I/O, which incidentally allows it to be run on some DSPs that don't support 8 bit bytes.


# Building OTEAX

To build OTEAX on your desktop and run tests, you can do simply:

```
$ make all
$ make install
```

This assumes that you have a GNU or GNU-like toolchain.  If you're using Windows, first install a proper operating system and then repeat above.

## Where Library Gets Installed

OTEAX uses the HBuilder Package management schema.  In the directory below the OTEAX directory, an `_hbpkg` directory will be created, and the contents installed into subdirectories within.  If you're not building other products with the HBuilder Package management schema, you can skip the `make install` stage, and just manually copy the library and header from the OTEAX directory, after build.

## Building for Cross Compiler

You may need to edit the makefile in order to point it to your toolchain directories, but other than this it is straightforward to build to a different target than the current machine.  The basic syntax is shown below:

```
$ make lib TARGET=[target_name]
```

Where you would change `[target_name]` with the target name of your choosing.  Targets supported in the makefile at time of writing include:

* `this` : build for local machine (same as no TARGET)
* `c2000` : generic TI C2000 device
* `arm-cm0` : generic ARM Cortex M0 / M0 Plus
* `arm-cm3` : generic ARM Cortex M3
* `stm32l0` : STM32L0 series
* `stm32l1` : STM32L1 series

## Architectural Optimizations

Beyond compiler optimizations, architectural optimizations can be made to the OTEAX code.  In a nutshell, there are various permutations of algorithm optimizations, data optimizations, and runtime optimizations that can be selected in `aes_opt.h`. The OTEAX build process simplifies this considerably.

```
$ make lib OPTIMIZE=[normal|size|speed]
```

* `size` : optimize for small code size. Lookup tables are not used.  Loop unrolling is used sparingly.
* `speed` : optimize for maximum runtime speed. Lookup tables and loop unrolling are used whenever possible. 
* `normal` : strikes a balance between size and speed.

Real benchmark data is TBD, but in terms of library size using gcc on ARM Cortex-M3, for example, size optimization has a 16KB library, normal is 26KB, and speed is 43KB.

## Static or Dynamic Library

You can build OTEAX as a static (.a) or shared/dynamic (.so, .dylib) library.  The default library type depends on the target.

* Embedded Target (STM32, C2000, etc) : Default is Static
* Desktop Target (Linux, Mac, etc): Default is Shared/Dynamic

If you want to override the default library type, you can provide the following input (below). Keep in mind that for some targets, building dynamic library may not be possible.

```
$ make lib LIBTYPE=[static|dynamic]
```

## Special Features

There are some special features that may be injected into the compilation process by using the `EXT_DEF` variable.  `EXT_DEF` passes definitions into the compiler, like such:

```
$ make lib EXT_DEF=-D__ALIGN32__
```

In the example above, the `__ALIGN32__` preprocessor variable gets passed into compilation.  Here is a list of variables you can pass at present:

* `__ALIGN32__` : Compile OTEAX to work with 32 bit aligned input and output.  This is used by default (automatically) on C2000 builds.
* `__OPENTAG__` : Build OTEAX to be integrated with OpenTag.  This will use OpenTag API functions instead of STDC or POSIX variants, when it makes sense.
* [no more yet]



# Including OTEAX Into Your Project

## Dependencies

OTEAX has no dependencies other than:

* stdint.h
* memcpy, memset, memcmp (often in string.h)

If you have special variants of memcpy, memset, memcmp that you would prefer to use instead of the ones from string.h, you can inform the build process by using (don't type the brackets):

```
$ make all TARGET=[target_name] STRING_H=[path to your string.h]
```

## Header

As with any project, you will need to make sure building & linking directories are setup in your compiler.  After that, use the following include declaration to allow linking to the top-level functions.  You don't need to include any other files.

```
#include <oteax.h>
```

## Alternatively, Building OTEAX Together With Your Project

Typical usage is to build liboteax.a and it to your embedded project along with oteax.h, however you can also add the OTEAX code directly into your project if you choose.  All of the important code is in the `./main` directory, and you can just copy it to a build directory in your project.  Make sure to put `oteax.h` into an include directory the whole project can see, though.





