
INCDIR = ./

#NOTE: this build is so small that the source and objects are 
#      easy enough to supply explicitly.
OTEAXLIB_C := ./aes_modes_m2.c ./aescrypt.c ./aeskey.c ./aestab.c ./oteax.c

TEST_C := _testmain.c
TEST_O := _testmain.o


# STM32 BUILD (LIB)
STM32_DEFINES := -MD -mthumb -mcpu=cortex-m3 -D_STM3x_ -D_STM32x_ -D__opentag__ -D__LITTLE_ENDIAN__
STM32_CC      := arm-none-eabi-gcc
STM32_CFLAGS  := -O2
STM32_OBJECTS  := $(patsubst ./%,build/stm32_m3/%,$(OTEAXLIB_C:.c=.o))

# NATIVE BUILD (LIB)
CC       := gcc
DEFINES  := -D__opentag__ -D__LITTLE_ENDIAN__
CFLAGS   := -O3
OBJECTS  := $(patsubst ./%,build/native/%,$(OTEAXLIB_C:.c=.o))

#Derived Variables
INC := -I$(INCDIR)


test: oteax_test_out 
lib: oteax_lib_a

all: test lib lib_stm32

clean:
	rm -f *.o 
	rm -f *.gch

wipe: clean
	rm -f *.a
	rm -f oteax_test

install: clean


oteax_test_out: oteax_test_o oteax_lib_o $(TEST_O) $(OTEAXLIB_O)
	$(COMPILER) $(DEFINES) $(INCLUDES) -o oteax_test $(TEST_O) $(OTEAXLIB_O)

oteax_lib_a: $(OTEAXLIB_O)
	libtool -o liboteax.a -static $(OTEAXLIB_O)

oteax_test_o: $(TEST_C)
	$(COMPILER) $(FLAGS) $(DEFINES) $(INCLUDES) -c $(TEST_C)

oteax_lib_o: $(OTEAXLIB_C)
	$(COMPILER) $(FLAGS) $(DEFINES) $(INCLUDES) -c $^
	


