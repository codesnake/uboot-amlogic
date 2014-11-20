#
# Copyright (C) 2008-2010 ARM Limited                           
#
# This software is provided 'as-is', without any express or implied
# warranties including the implied warranties of satisfactory quality, 
# fitness for purpose or non infringement.  In no event will  ARM be 
# liable for any damages arising from the use of this software.
#
# Permission is granted to anyone to use, copy and modify this software for 
# any purpose, and to redistribute the software, subject to the following 
# restrictions: 
#
# 1. The origin of this software must not be misrepresented; you must not
#    claim that you wrote the original software. If you use this software
#    in a product, an acknowledgment in the product documentation would be
#    appreciated but is not required.                                       
# 2. Altered source versions must be plainly marked as such, and must not be
#    misrepresented as being the original software.
# 3. This notice may not be removed or altered from any source distribution.
#
include $(TOPDIR)/config.mk
OUTDIR = $(SRCTREE)/build/appf
EXTINCDIR=$(SRCTREE)/build/include2
EXTINCDIR2=$(SRCTREE)/build/include
EXTINCDIR3=$(SRCTREE)/include
PLATFORM?=a9-eb
ENTRY_POINT=appf_boottime_init
LISTFILE=$(OUTDIR)/symbols.txt
HEADERS=appf_boot_api.h appf_internals.h appf_types.h \
        appf_helpers.h appf_platform_api.h

OBJECTS= $(OUTDIR)/os_api.o \
         $(OUTDIR)/entry.o \
         $(OUTDIR)/translation.o \
         $(OUTDIR)/table_data.o \
         $(OUTDIR)/helpers.o \
				 $(OUTDIR)/gic.o \
				 $(OUTDIR)/pl310.o \
				 $(OUTDIR)/v7.o \
				 $(OUTDIR)/a9.o \
				 $(OUTDIR)/a9_c.o \
				 $(OUTDIR)/scu.o \
				 $(OUTDIR)/c_helpers.o \
				 $(OUTDIR)/platform/$(PLATFORM)/platform.o \
				 $(OUTDIR)/platform/$(PLATFORM)/power.o \
	       $(OUTDIR)/platform/$(PLATFORM)/context.o \
         $(OUTDIR)/platform/$(PLATFORM)/reset.o \
         $(OUTDIR)/serial.o
        
CPU=cortex-a9
GCC= TRUE
ifeq "$(GCC)" "TRUE"
CC=arm-none-eabi-gcc
AS=arm-none-eabi-gcc
LD=arm-none-eabi-ld
fromelf=arm-none-eabi-objcopy
MAPFILE=appf.gls
CFLAGS= -g -O2 -fpie -fpic -mcpu=$(CPU) -I. -DNO_PCU -I$(EXTINCDIR) -I$(EXTINCDIR2) -I$(EXTINCDIR3) 
# -DUSE_REALVIEW_EB_RESETS
ASFLAGS= -g -mcpu=$(CPU) -mfpu=vfpv3 -c
LDFLAGS= -g -nostartfiles -pie -fpic -static -e $(ENTRY_POINT) --verbose -T $(MAPFILE) -M

$(OUTDIR)/%.o : $(OUTDIR)/%.S	# Remove make's implicit rule for .S files
.PRECIOUS : $(OUTDIR)/%.gnu.S
$(OUTDIR)/%.gnu.S : %.S
		perl arm_to_gnu.pl $< $@
$(OUTDIR)/%.o : $(OUTDIR)/%.gnu.S Makefile $(MAPFILE) $(HEADERS)
		$(AS) $(ASFLAGS) $< -o $@

else

CC=armcc
AS=armasm
LD=armlink
MAPFILE=appf.scf
CFLAGS= -g  -O2 --cpu=$(CPU) --fpu=none -I. --apcs=/fpic --gnu -DNO_PCU 
# -DUSE_REALVIEW_EB_RESETS
ASFLAGS= --keep -g  --cpu $(CPU) --fpu=none --apcs=/fpic 
LDFLAGS= --verbose --map --fpu=none --symbols --noremove --datacompressor=off --fpic --entry $(ENTRY_POINT) --scatter $(MAPFILE)

%.o : %.S Makefile $(MAPFILE) $(HEADERS)
		$(AS) $(ASFLAGS) $< -o $@


endif

all: clean $(OUTDIR)/appf.axf $(OUTDIR)/appf.bin $(OUTDIR)/appf_srec.txt

clean:
	rm -f $(OBJECTS)
	rm -f $(LISTFILE)
	rm -f $(OUTDIR)/appf.axf
	rm -f $(OUTDIR)/appf.bin
	rm -f $(OUTDIR)/appf_srec.txt
	rm -f $(EXTINCDIR)/power_firmware.dat
	mkdir -p $(OUTDIR)/platform/$(PLATFORM)
	
$(OUTDIR)/%.o : %.c Makefile $(MAPFILE) $(HEADERS)
		$(CC) $(CFLAGS) -c $< -o $@

$(OUTDIR)/appf.axf: $(OBJECTS) $(MAPFILE) Makefile
	$(LD) $(LDFLAGS) $(OBJECTS) -o $@ > $(LISTFILE)

$(OUTDIR)/appf.bin: $(OUTDIR)/appf.axf
	$(fromelf) -O binary $<  $@
	@echo "===================="
	./bin2code power_firmware_code $@ $(EXTINCDIR)/power_firmware.dat

ifeq "$(FIRMWARE_AUTO_UPDATE)" "y"
#	cp -f $(OUTDIR)/power_firmware.dat $(SRCTREE)/arch/arm/cpu/aml_meson/m6tv/power_firmware.dat
endif

$(OUTDIR)/appf_srec.txt: $(OUTDIR)/appf.axf
	$(fromelf) --gap-fill 0xff -O srec $< $@
	
