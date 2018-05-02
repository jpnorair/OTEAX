# Default Configuration
TARGET      ?= $(shell uname -srm | sed -e 's/ /-/g')
PRODUCT     ?= oteax
OPTIMIZE    ?= normal
VERSION     ?= "1.2.0"
EXT_DEF     ?= 
EXT_INC     ?= 
EXT_LIBS    ?= 

LIBNAME     := lib$(PRODUCT)
DEFAULT_INC := ./
LIBMODULES  := 
SUBMODULES  := main
BUILDDIR    := ./build
SRCEXT      := c
DEPEXT      := d
OBJEXT      := o
THISMACHINE := $(shell uname -srm | sed -e 's/ /-/g')
THISSYSTEM	:= $(shell uname -s)


# Conditional Architectural Optimization Settings
ifeq ($(OPTIMIZE),size)
    OPTIM_DEF   := -DOTEAX_OPTIMIZATION=-1
else ifeq ($(OPTIMIZE),speed)
    OPTIM_DEF   := -DOTEAX_OPTIMIZATION=2
else
    OPTIM_DEF   := -DOTEAX_OPTIMIZATION=0
endif

# Handle pseudo-explicit "this" TARGET
ifeq ($(TARGET),this)
	X_TARG      := $(THISMACHINE)
else
	X_TARG      := $(TARGET)
endif

# Conditional Settings per Target
ifeq ($(X_TARG),$(THISMACHINE))
	X_PRDCT     := $(LIBNAME).$(THISSYSTEM)
	X_PKGDIR    ?= ./../_hbpkg/$(X_TARG)/$(LIBNAME).$(VERSION)
	X_CC	    := gcc
	X_LIBTOOL   := libtool
	X_CFLAGS    := -std=gnu99 -fPIC -O3 -pthread
	X_DEF       := $(OPTIM_DEF) $(EXT_DEF)
	X_INC       := -I$(DEFAULT_INC) $(EXT_INC)
	X_LIB       := -Wl,-Bstatic $(EXT_LIBS)
	X_PLAT      := ./platform/posix_c

else ifeq ($(X_TARG),c2000)
    # These two paths may need to be changed depending on your platform.
    C2000_WARE  ?= /Applications/ti/c2000/C2000Ware_1_00_02_00
	TICC_DIR    ?= /Applications/ti/ccsv7/tools/compiler/ti-cgt-c2000_17.9.0.STS
	
	X_PRDCT     := $(LIBNAME).c2000
	X_PKGDIR    ?= ./../_hbpkg/$(X_TARG)/$(LIBNAME).$(VERSION)
	X_CC	    := cl2000
	X_LIBTOOL   := ar2000
	X_CFLAGS    := --c99 -O2 -v28 -ml -mt -g --cla_support=cla0 --float_support=fpu32 --vcu_support=vcu0 
	X_DEF       := $(OPTIM_DEF) -DAPP_csip_c2000 -DBOARD_C2000_null -D__TMS320F2806x__ -D__C2000__ -D__TI_C__ -D__NO_SECTIONS__ $(EXT_DEF)
	X_INC       := -I$(TICC_DIR)/include -I$(C2000_WARE) -I$(DEFAULT_INC) $(EXT_INC)
	X_LIB       := -Wl,-Bstatic -L$(TICC_DIR)/lib -L./ $(EXT_LIBS)
	X_PLAT      := ./platform/c2000

else
	error "TARGET set to unknown value: $(X_TARG)"
endif

# Export the following variables to the shell: will affect submodules
export X_CC
export X_CFLAGS
export X_DEF
export X_INC
export X_LIB


# Global vars that get exported to sub-makefiles
all: $(X_PRDCT) test
lib: $(X_PRDCT)
remake: cleaner all

install: 
	@mkdir -p $(X_PKGDIR)
	@cp ./pkg/* ./$(X_PKGDIR)
	@rm -f $(X_PKGDIR)/../$(LIBNAME)
	@ln -s $(LIBNAME).$(VERSION) ./$(X_PKGDIR)/../$(LIBNAME)
	
directories:
	@mkdir -p pkg
	@mkdir -p $(BUILDDIR)

#Clean only Objects
clean:
	@$(RM) -rf $(BUILDDIR)

#Full Clean, Objects and Binaries
cleaner: clean
	@$(RM) -rf pkg

# Test
test: $(X_PRDCT)
	$(eval MKFILE := $(notdir $@))
	cd ./$@ && $(MAKE) -f $(MKFILE).mk obj

#Packaging stage: copy/move files to pkg output directory
$(X_PRDCT): $(X_PRDCT).lib
	@cp ./main/$(PRODUCT).h ./pkg
	@cp -R ./main/oteax ./pkg/

#Build the static library
#There are several supported variants.
$(LIBNAME).Darwin.lib: $(SUBMODULES) $(LIBMODULES)
	$(eval LIBTOOL_OBJ := $(shell find $(BUILDDIR) -type f -name "*.$(OBJEXT)"))
#	$(X_LIBTOOL) -static -o $(LIBNAME).a $(LIBTOOL_OBJ)
#	@mv $(LIBNAME).a pkg/
	$(X_CC) -dynamiclib -o pkg/$(LIBNAME).dylib $(LIBTOOL_OBJ)

$(LIBNAME).Linux.lib: $(SUBMODULES) $(LIBMODULES)
	$(eval LIBTOOL_OBJ := $(shell find $(BUILDDIR) -type f -name "*.$(OBJEXT)"))
#	$(X_LIBTOOL) --tag=CC --mode=link $(X_CC) -all-static -g -O3 $(X_INC) $(X_LIB) -o $(LIBNAME).a $(LIBTOOL_OBJ)
#	@mv $$(LIBNAME).a pkg/
	$(X_CC) -shared -o pkg/$(LIBNAME).so $(LIBTOOL_OBJ)

$(LIBNAME).c2000.lib: $(SUBMODULES) $(LIBMODULES)
	$(eval LIBTOOL_OBJ := $(shell find $(BUILDDIR)/$(LIBNAME).c2000 -type f -name "*.$(OBJEXT)"))
	ar2000 -a $(LIBNAME).a $(LIBTOOL_OBJ)
	@mv $(LIBNAME).a pkg/

#Library dependencies (not in oteax sources)
$(LIBMODULES): %: 
	cd ./../$@ && $(MAKE) all

# Library Code
$(SUBMODULES): %: $(LIBMODULES) directories
	$(eval MKFILE := $(notdir $@))
	cd ./$@ && $(MAKE) -f $(MKFILE).mk obj

#Non-File Targets
.PHONY: all lib remake clean cleaner

