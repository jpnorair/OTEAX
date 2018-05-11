TARGET      := test

X_CC	    ?= gcc
X_CFLAGS    ?= -std=gnu99 -O3
X_DEF       ?= 
X_INC       ?= 
X_LIB       ?= 

BUILDDIR    := ../build_$(TARGET)
PRODUCTDIR  := ../bin
SRCEXT      := c
DEPEXT      := d
OBJEXT      := o
APPEXT      := out
LIB         := -loteax -L./../pkg $(patsubst -L./%,-L./../%,$(X_LIB))
INC         := -I./../pkg $(patsubst -I./%,-I./../%,$(X_INC))
INCDEP      := $(INC)

SOURCES     := $(shell find . -type f -name "*.$(SRCEXT)")
OBJECTS     := $(patsubst ./%,$(BUILDDIR)/%,$(SOURCES:.$(SRCEXT)=.$(OBJEXT)))
PRODUCTS    := $(patsubst $(BUILDDIR)/%,$(PRODUCTDIR)/%,$(OBJECTS:.$(OBJEXT)=.$(APPEXT)))

# Need to specify compiler input flags because TI compiler is stupid (doesn't abide by documentation)
ifeq ($(X_CC),gcc)
	CCOUT = -o 
else
    # TI Compiler
	CCOUT = --output_file=
endif


all: resources $(PRODUCTS)
obj: $(OBJECTS)
remake: clean all


#Copy Resources from Resources Directory to Target Directory
resources: directories

#Make the Directories
directories:
	@mkdir -p $(PRODUCTDIR)
	@mkdir -p $(BUILDDIR)

#Clean only Objects
clean:
	@$(RM) -rf $(BUILDDIR)

#Pull in dependency info for *existing* .o files
-include $(OBJECTS:.$(OBJEXT)=.$(DEPEXT))

#Direct build of the test app with objects
$(PRODUCTDIR)/%.$(APPEXT): $(BUILDDIR)/%.$(OBJEXT)
	$(X_CC) $(LIB) $(INC) -o $@ $<
	

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
.PHONY: all remake clean resources

