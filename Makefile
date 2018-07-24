# Default Configuration
TARGET      ?= $(shell uname -srm | sed -e 's/ /-/g')
PRODUCT     ?= oteax
OPTIMIZE    ?= normal
LIBTYPE     ?= 
VERSION     ?= 1.2.0
EXT_DEF     ?= 
EXT_INC     ?= 
EXT_LIBS    ?= 

LIBNAME     := lib$(PRODUCT)
DEFAULT_INC := ./
LIBMODULES  := 
SUBMODULES  := main
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

# BUILDDIR must be set after settling the target type
BUILDDIR    := ./build/$(X_TARG)
PRODUCTDIR  := ./bin/$(X_TARG)

# Conditional Settings per Target
ifeq ($(X_TARG),$(THISMACHINE))
	ifeq ($(THISSYSTEM),Darwin)
	# Mac can't do conditional selection of static and dynamic libs at link time.
	#	PRODUCT_LIBS := liboteax.$(THISSYSTEM).dylib liboteax.$(THISSYSTEM).a
		PRODUCT_LIBS := liboteax.$(THISSYSTEM).a
	else ifeq ($(THISSYSTEM),Linux)
		PRODUCT_LIBS := liboteax.$(THISSYSTEM).so liboteax.$(THISSYSTEM).a
	else
		error "THISSYSTEM set to unknown value: $(THISSYSTEM)"
	endif
	X_PRDCT     := $(LIBNAME).$(THISSYSTEM)
	X_PKGDIR    ?= ./../_hbpkg/$(X_TARG)/$(LIBNAME).$(VERSION)
	X_CC	    := gcc
	X_LIBTOOL   := libtool
	X_CFLAGS    := -std=gnu99 -O3 -pthread -fPIC
	X_DEF       := $(OPTIM_DEF) $(EXT_DEF)
	X_INC       := -I$(DEFAULT_INC) $(EXT_INC)
	X_LIB       := $(EXT_LIBS)
	X_PLAT      := ./platform/posix_c

else ifeq ($(X_TARG),c2000)
    # These two paths may need to be changed depending on your platform.
    ifdef C2000_WARE
    # C2000_WARE is set as environment variable
    else ifeq ($(THISSYSTEM),Darwin)
        C2000_WARE  ?= /Applications/ti/c2000/C2000Ware_1_00_02_00
	else ifeq ($(THISSYSTEM),Linux)
		C2000_WARE  ?= /opt/ti/c2000/C2000Ware_1_00_02_00
	else ifeq ($(THISSYSTEM),CYGWIN_NT-10.0)
	    C2000_WARE  ?= C:/ti/c2000/C2000Ware_1_00_04_00
	else
		error "THISSYSTEM set to unknown value: $(THISSYSTEM)"
	endif
    ifdef TICC_DIR
    # TICC_DIR is set as environment variable
    else ifeq ($(THISSYSTEM),Darwin)
	    TICC_DIR    ?= /Applications/ti/ccsv7/tools/compiler/ti-cgt-c2000_17.9.0.STS
	else ifeq ($(THISSYSTEM),Linux)
	    TICC_DIR    ?= /opt/ti/ccsv7/tools/compiler/ti-cgt-c2000_17.9.0.STS
	else ifeq ($(THISSYSTEM),CYGWIN_NT-10.0)
	    TICC_DIR    ?= C:/ti/ccsv8/tools/compiler/ti-cgt-c2000_18.1.2.LTS
	else
		error "THISSYSTEM set to unknown value: $(THISSYSTEM)"
	endif
    
	X_PRDCT     := $(LIBNAME).c2000
	X_PKGDIR    ?= ./../_hbpkg/$(X_TARG)/$(LIBNAME).$(VERSION)
	X_CC	    := cl2000
	X_LIBTOOL   := ar2000
	X_CFLAGS    := --c99 -O2 -v28 -ml -mt -g --cla_support=cla0 --float_support=fpu32 --vcu_support=vcu0 
	X_DEF       := $(OPTIM_DEF) -DAPP_csip_c2000 -DBOARD_C2000_null -D__TMS320F2806x__ -D__C2000__ -D__ALIGN32__ -D__TI_C__ -D__NO_SECTIONS__ $(EXT_DEF)
	X_INC       := -I$(TICC_DIR)/include -I$(C2000_WARE) -I$(DEFAULT_INC) $(EXT_INC)
	X_LIB       := -Wl,-Bstatic -L$(TICC_DIR)/lib -L./ $(EXT_LIBS)
	X_PLAT      := ./platform/c2000
	PRODUCT_LIBS:= liboteax.c2000.a

else
	error "TARGET set to unknown value: $(X_TARG)"
endif

# Export the following variables to the shell: will affect submodules
export X_CC
export X_CFLAGS
export X_DEF
export X_INC
export X_LIB
export X_TARG

# Global vars that get exported to sub-makefiles
all: $(X_PRDCT) test
lib: $(X_PRDCT)
remake: cleaner all
pkg: lib install


install: 
	@rm -rf $(X_PKGDIR)
	@mkdir -p $(X_PKGDIR)
	@cp -R $(PRODUCTDIR)/* $(X_PKGDIR)
	@rm -f $(X_PKGDIR)/../$(LIBNAME)
	@ln -s $(LIBNAME).$(VERSION) ./$(X_PKGDIR)/../$(LIBNAME)
	cd ../_hbsys && $(MAKE) sys_install INS_MACHINE=$(TARGET) INS_PKGNAME=liboteax
	
directories:
	@rm -rf $(PRODUCTDIR)
	@mkdir -p $(PRODUCTDIR)
	@mkdir -p $(BUILDDIR)

# Clean only this machine target
clean:
	@$(RM) -rf $(BUILDDIR)
	@$(RM) -rf $(PRODUCTDIR)

# Clean all machine targets
cleaner: 
	@$(RM) -rf ./bin
	@$(RM) -rf ./build

# Test
test: $(X_PRDCT)
	$(eval MKFILE := $(notdir $@))
	cd ./$@ && $(MAKE) -f $(MKFILE).mk all

#Packaging stage: copy/move files to output directory
$(X_PRDCT): $(PRODUCT_LIBS)
	@cp ./main/$(PRODUCT).h $(PRODUCTDIR)
	@cp -R ./main/oteax $(PRODUCTDIR)/



# Build the library
# There are several supported variants.
# - Mac: static and dynamic
# - Linux: static and dynamic
# - C2000: static
# - STM32: static (todo)
$(LIBNAME).Darwin.a: $(SUBMODULES) $(LIBMODULES)
	$(eval LIBTOOL_OBJ := $(shell find $(BUILDDIR) -type f -name "*.$(OBJEXT)"))
	$(X_LIBTOOL) -static -o $(LIBNAME).a $(LIBTOOL_OBJ)
	@mv $(LIBNAME).a $(PRODUCTDIR)/

$(LIBNAME).Darwin.dylib: $(SUBMODULES) $(LIBMODULES)
	$(eval LIBTOOL_OBJ := $(shell find $(BUILDDIR) -type f -name "*.$(OBJEXT)"))
	$(X_CC) -dynamiclib -o $(LIBNAME).dylib $(LIBTOOL_OBJ)
	@mv $(LIBNAME).dylib $(PRODUCTDIR)/

$(LIBNAME).Linux.a: $(SUBMODULES) $(LIBMODULES)
	$(eval LIBTOOL_OBJ := $(shell find $(BUILDDIR) -type f -name "*.$(OBJEXT)"))
	ar rcs $(LIBNAME).a $(LIBTOOL_OBJ)
	ranlib $(LIBNAME).a
#	$(X_LIBTOOL) --tag=CC --mode=link $(X_CC) -all-static -g -O3 $(X_INC) $(X_LIB) -o $(LIBNAME).a $(LIBTOOL_OBJ)
	@mv $(LIBNAME).a $(PRODUCTDIR)/
	
$(LIBNAME).Linux.so: $(SUBMODULES) $(LIBMODULES)
	$(eval LIBTOOL_OBJ := $(shell find $(BUILDDIR)/main -type f -name "*.$(OBJEXT)"))
	$(X_CC) -shared -fPIC -Wl,-soname,$(LIBNAME).so.1 -o $(LIBNAME).so.$(VERSION) $(LIBTOOL_OBJ) -lc
	@mv $(LIBNAME).so.$(VERSION) $(PRODUCTDIR)/
	
$(LIBNAME).c2000.a: $(SUBMODULES) $(LIBMODULES)
	$(eval LIBTOOL_OBJ := $(shell find $(BUILDDIR) -type f -name "*.$(OBJEXT)"))
	ar2000 -a $(LIBNAME).a $(LIBTOOL_OBJ)
	@mv $(LIBNAME).a $(PRODUCTDIR)/



#Library dependencies (not in oteax sources)
$(LIBMODULES): %: 
	cd ./../$@ && $(MAKE) all

# Library Code
$(SUBMODULES): %: $(LIBMODULES) directories
	$(eval MKFILE := $(notdir $@))
	cd ./$@ && $(MAKE) -f $(MKFILE).mk obj

#Non-File Targets
.PHONY: all lib remake test clean cleaner

