CC := gcc
LD := ld

GROUP     := main

X_CC	    ?= $(CC)
X_CFLAGS    ?= -std=gnu99 -O3
X_DEF       ?= 
X_INC       ?= 
X_LIB       ?= 
X_TARG	 	?= default

BUILDDIR    := ../build/$(X_TARG)/$(GROUP)
GROUPDIR    := .
SRCEXT      := c
DEPEXT      := d
OBJEXT      := o
LIB         := $(patsubst -L./%,-L./../%,$(X_LIB)) 
INC         := $(patsubst -I./%,-I./../%,$(X_INC)) 
INCDEP      := $(INC)

SOURCES     := $(shell find . -type f -name "*.$(SRCEXT)")
OBJECTS     := $(patsubst ./%,$(BUILDDIR)/%,$(SOURCES:.$(SRCEXT)=.$(OBJEXT)))

# Need to specify compiler input flags because TI compiler is stupid (doesn't abide by documentation)
ifneq (,$(findstring gcc,$(X_CC)))
	CCOUT = -o 
else
    # TI Compiler
	CCOUT = --output_file=
endif


all: resources $(GROUP)
obj: $(OBJECTS)
remake: cleaner all


#Copy Resources from Resources Directory to Target Directory
resources: directories

#Make the Directories
directories:
	@mkdir -p $(GROUPDIR)
	@mkdir -p $(BUILDDIR)

#Clean only Objects
clean:
	@$(RM) -rf $(BUILDDIR)

#Full Clean, Objects and Binaries
cleaner: clean
	@$(RM) -rf $(GROUPDIR)

#Pull in dependency info for *existing* .o files
-include $(OBJECTS:.$(OBJEXT)=.$(DEPEXT))

#Direct build of the test app with objects
$(GROUP): $(OBJECTS)
	$(X_CC) -o $(GROUPDIR)/$(GROUP) $^ $(LIB)

#Compile Stages
$(BUILDDIR)/%.$(OBJEXT): ./%.$(SRCEXT)
	@mkdir -p $(dir $@)
ifeq ($(X_CC),gcc)
	$(X_CC) $(X_CFLAGS) $(X_DEF) $(INC) -c -o $@ $<
	@$(X_CC) $(X_CFLAGS) $(X_DEF) $(INCDEP) -MM ./$*.$(SRCEXT) > $(BUILDDIR)/$*.$(DEPEXT)
	@cp -f $(BUILDDIR)/$*.$(DEPEXT) $(BUILDDIR)/$*.$(DEPEXT).tmp
	@sed -e 's|.*:|$(BUILDDIR)/$*.$(OBJEXT):|' < $(BUILDDIR)/$*.$(DEPEXT).tmp > $(BUILDDIR)/$*.$(DEPEXT)
	@sed -e 's/.*://' -e 's/\\$$//' < $(BUILDDIR)/$*.$(DEPEXT).tmp | fmt -1 | sed -e 's/^ *//' -e 's/$$/:/' >> $(BUILDDIR)/$*.$(DEPEXT)
	@rm -f $(BUILDDIR)/$*.$(DEPEXT).tmp
else
	$(X_CC) $(X_CFLAGS) $(X_DEF) $(INC) -c $(CCOUT)$@ $<
endif

#Non-File Targets
.PHONY: all remake clean cleaner resources

