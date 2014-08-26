COMPILER=gcc

INC = ./

#OT_HEADERS := $(OTLIB)/$(wildcard *.h)
#PL_HEADERS := $(PLATFORM)/$(wildcard *.h)
#NOTE: I don't use wildcards in the build strings, because I like to keep the
#      compilations selective.

TEST_C = _testmain.c
TEST_O = _testmain.o

OTEAXLIB_C = aes_modes_m2.c aescrypt.c aeskey.c aestab.c oteax.c
OTEAXLIB_O = aes_modes_m2.o aescrypt.o aeskey.o aestab.o oteax.o

INCLUDES = -I$(INC)
FLAGS = -O2

test: oteax_test_out 
lib: oteax_lib_a

all: test lib

clean:
	rm -f *.o 
	rm -f *.gch

wipe: clean
	rm -f *.a
	rm -f oteax_test

install: clean


oteax_test_out: oteax_test_o oteax_lib_o $(TEST_O) $(OTEAXLIB_O)
	$(COMPILER) $(INCLUDES) -o oteax_test $(TEST_O) $(OTEAXLIB_O)

oteax_lib_a: $(OTEAXLIB_O)
	libtool -o liboteax.a -static $(OTEAXLIB_O)

oteax_test_o: $(TEST_C)
	$(COMPILER) $(FLAGS) $(INCLUDES) -c $(TEST_C)

oteax_lib_o: $(OTEAXLIB_C)
	$(COMPILER) $(FLAGS) $(INCLUDES) -c $(OTEAXLIB_C)

